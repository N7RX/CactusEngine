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

DrawingCommandManager_Vulkan::DrawingCommandManager_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue)
	: m_pDevice(pDevice), m_workingQueue(queue), m_commandPool(VK_NULL_HANDLE)
{

}

DrawingCommandManager_Vulkan::~DrawingCommandManager_Vulkan()
{
	vkDestroyCommandPool(m_pDevice->GetLogicalDevice(), m_commandPool, nullptr);
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

	vkCreateCommandPool(m_pDevice->GetLogicalDevice(), &poolInfo, nullptr, &m_commandPool);
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

	if (vkAllocateCommandBuffers(m_pDevice->GetLogicalDevice(), &allocInfo, cmdBufferHandles.data()) == VK_SUCCESS)
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