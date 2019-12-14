#include "DrawingCommandManager_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>

using namespace Engine;

DrawingCommandBuffer_Vulkan::DrawingCommandBuffer_Vulkan(const VkCommandBuffer& cmdBuffer)
	: m_isRecording(false), m_inRenderPass(false), m_inExecution(false), m_fenceID(-1), m_commandBuffer(cmdBuffer)
{

}

DrawingCommandBuffer_Vulkan::~DrawingCommandBuffer_Vulkan()
{

}

bool DrawingCommandBuffer_Vulkan::IsRecording() const
{
	return m_isRecording;
}

bool DrawingCommandBuffer_Vulkan::InRenderPass() const
{
	return m_inRenderPass;
}

bool DrawingCommandBuffer_Vulkan::InExecution() const
{
	return m_inExecution;
}

DrawingCommandManager_Vulkan::DrawingCommandManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue)
	: m_pDevice(pDevice), m_workingQueue(queue), m_commandPool(VK_NULL_HANDLE)
{
	m_frameFenceLock = std::unique_lock<std::mutex>(m_frameFenceMutex, std::defer_lock);
	m_frameFenceLock.lock();
	m_commandBufferSubmissionThread = std::thread(&DrawingCommandManager_Vulkan::SubmitCommandBufferAsync, this);

	m_commandBufferRecycleThread = std::thread(&DrawingCommandManager_Vulkan::RecycleCommandBufferAsync, this);

	m_isRunning = true;
}

DrawingCommandManager_Vulkan::~DrawingCommandManager_Vulkan()
{
	m_isRunning = false;
	Destroy();
}

void DrawingCommandManager_Vulkan::Destroy()
{
	vkDestroyCommandPool(m_pDevice->logicalDevice, m_commandPool, nullptr);
}

EQueueType DrawingCommandManager_Vulkan::GetWorkingQueueType() const
{
	return m_workingQueue.type;
}

std::shared_ptr<DrawingCommandBuffer_Vulkan> DrawingCommandManager_Vulkan::RequestPrimaryCommandBuffer()
{
	std::shared_ptr<DrawingCommandBuffer_Vulkan> commandBuffer;

	if (m_freeCommandBuffers.Size() == 0)
	{
		AllocatePrimaryCommandBuffer(1); // We can allocate more at once if more will be needed
	}

	m_freeCommandBuffers.TryPop(commandBuffer);
	m_inUseCommandBuffers.Push(commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We only submit the command buffer once

	if (vkBeginCommandBuffer(commandBuffer->m_commandBuffer, &beginInfo) == VK_SUCCESS)
	{
		commandBuffer->m_isRecording = true;
		return commandBuffer;
	}

	throw std::runtime_error("Vulkan: Failed to begin command buffer.");
	return nullptr;
}

void DrawingCommandManager_Vulkan::SubmitCommandBuffers(VkFence fence)
{

}

void DrawingCommandManager_Vulkan::CreateCommandPool()
{
	assert(m_commandPool == VK_NULL_HANDLE);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_workingQueue.queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Alert: all command buffers allocated from this pool can be reset

	vkCreateCommandPool(m_pDevice->logicalDevice, &poolInfo, nullptr, &m_commandPool);
}

bool DrawingCommandManager_Vulkan::AllocatePrimaryCommandBuffer(uint32_t count)
{
	assert(m_freeCommandBuffers.Size() + m_inUseCommandBuffers.Size() + m_inExecutionCommandBuffers.Size() + count <= MAX_COMMAND_BUFFER_COUNT);

	std::vector<VkCommandBuffer> cmdBufferHandles(count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	if (vkAllocateCommandBuffers(m_pDevice->logicalDevice, &allocInfo, cmdBufferHandles.data()) == VK_SUCCESS)
	{
		for (auto& cmdBuffer : cmdBufferHandles)
		{
			m_freeCommandBuffers.Push(std::make_shared<DrawingCommandBuffer_Vulkan>(cmdBuffer));
		}
		return true;
	}

	throw std::runtime_error("Vulkan: Failed to allocate command buffer.");
	return false;
}

void DrawingCommandManager_Vulkan::SubmitCommandBufferAsync()
{
	std::shared_ptr<CommandSubmitInfo> pCommandSubmitInfo;
	bool shouldRecycle = false;

	while (m_isRunning)
	{
		shouldRecycle = false;
		while (m_submissionQueue.TryPop(pCommandSubmitInfo))
		{
			vkQueueSubmit(m_workingQueue.queue, 1, &pCommandSubmitInfo->submitInfo, pCommandSubmitInfo->fence);

			if (pCommandSubmitInfo->waitFrameFenceSubmission)
			{
				m_frameFenceCv.notify_all();
			}

			if (pCommandSubmitInfo->initRecycle)
			{
				shouldRecycle = true;
			}

			pCommandSubmitInfo->Clear();

			if (shouldRecycle)
			{
				m_commandBufferRecycleFlag.AssignValue(true);
				m_commandBufferRecycleCv.notify_one();
			}

			std::this_thread::yield();
		}
	}
}

void DrawingCommandManager_Vulkan::RecycleCommandBufferAsync()
{
	std::unique_lock<std::mutex> lock(m_commandBufferRecycleMutex, std::defer_lock);
	std::unordered_map<uint32_t, std::vector<std::shared_ptr<DrawingCommandBuffer_Vulkan>>> fenceMappings;

	lock.lock(); // If the mutex object is not initially locked when waited, it would cause undefined behavior
	const int MAX_POPCOUNT = 32; // Prevent going into endless interleaved push-pop circle

	bool recycleFlag = false;
	int  popCount = 0;
	while (m_isRunning)
	{
		m_commandBufferRecycleFlag.GetValue(recycleFlag);
		if (!recycleFlag) // Check if there is any recycle notification during last recycle
		{
			m_commandBufferRecycleCv.wait(lock);
		}
		m_commandBufferRecycleFlag.AssignValue(false);

		std::shared_ptr<DrawingCommandBuffer_Vulkan> inExecutionCmdBuffer;
		popCount = 0;
		while (m_inExecutionCommandBuffers.TryPop(inExecutionCmdBuffer))
		{
			std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = inExecutionCmdBuffer;
			fenceMappings[ptrCopy->m_fenceID].emplace_back(ptrCopy);
			popCount++;
			if (popCount > MAX_POPCOUNT)
			{
				break;
			}
		}

		for (auto& fenceMap : fenceMappings)
		{
			if (fenceMap.second.size() > 0)
			{
				VkFence fenceToQuery = m_pDevice->pSyncObjectManager->RequestFenceByID(fenceMap.first);
				VkResult result = vkWaitForFences(m_pDevice->logicalDevice, 1, &fenceToQuery, VK_FALSE, RECYCLE_TIMEOUT);
				if (result == VK_SUCCESS)
				{
					for (auto pCmdBuffer : fenceMap.second)
					{
						pCmdBuffer->m_fenceID = -1;
						pCmdBuffer->m_inExecution = false;
					}
					m_pDevice->pSyncObjectManager->ReturnFenceByID(fenceMap.first);
				}
				else
				{
					throw std::runtime_error("Vulkan: command execution timeout.");
				}
				fenceMap.second.clear();
			}
		}

		std::this_thread::yield();
	}

	fenceMappings.clear();
}