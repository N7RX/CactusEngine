#include "DrawingCommandManager_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>

using namespace Engine;

DrawingCommandBuffer_Vulkan::DrawingCommandBuffer_Vulkan(const VkCommandBuffer& cmdBuffer)
	: m_isRecording(false), m_inRenderPass(false), m_inExecution(false), m_pAssociatedFence(nullptr), m_commandBuffer(cmdBuffer), m_pipelineLayout(VK_NULL_HANDLE)
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

void DrawingCommandBuffer_Vulkan::BeginCommandBuffer(VkCommandBufferUsageFlags usage)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = usage;

	VkResult result = vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
	assert(result == VK_SUCCESS);

	m_isRecording = true;
	m_inRenderPass = false;
	m_inExecution = false;
}

void DrawingCommandBuffer_Vulkan::BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer vertexBuffer, const VkDeviceSize offsets)
{
	assert(m_isRecording);
	vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, &vertexBuffer, &offsets);
}

void DrawingCommandBuffer_Vulkan::BindIndexBuffer(const VkBuffer indexBuffer, const VkDeviceSize offset, VkIndexType type)
{
	assert(m_isRecording);
	vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer, offset, type);
}

void DrawingCommandBuffer_Vulkan::BeginRenderPass(const VkRenderPass renderPass, const VkFramebuffer frameBuffer, const std::vector<VkClearValue>& clearValues, const VkExtent2D& areaExtent, const VkOffset2D& areaOffset)
{
	assert(m_isRecording);

	VkRenderPassBeginInfo passBeginInfo = {};
	passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passBeginInfo.renderPass = renderPass;
	passBeginInfo.renderArea.offset = areaOffset;
	passBeginInfo.renderArea.extent = areaExtent;

	passBeginInfo.clearValueCount = (uint32_t)clearValues.size();
	passBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	m_inRenderPass = true;
}

void DrawingCommandBuffer_Vulkan::BindPipeline(const VkPipelineBindPoint bindPoint, const VkPipeline pipeline)
{
	assert(m_isRecording);
	vkCmdBindPipeline(m_commandBuffer, bindPoint, pipeline);
}

void DrawingCommandBuffer_Vulkan::BindPipelineLayout(const VkPipelineLayout pipelineLayout)
{
	m_pipelineLayout = pipelineLayout;
}

void DrawingCommandBuffer_Vulkan::UpdatePushConstant(const VkShaderStageFlags shaderStage, uint32_t size, const void* pData, uint32_t offset)
{
	assert(m_isRecording);
	vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, shaderStage, offset, size, pData);
}

void DrawingCommandBuffer_Vulkan::BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet)
{
	assert(m_isRecording);
	vkCmdBindDescriptorSets(m_commandBuffer, bindPoint, m_pipelineLayout, firstSet, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);
	// Alert: dynamic offset is not handled
}

void DrawingCommandBuffer_Vulkan::DrawPrimitiveIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
	assert(m_inRenderPass);
	if (indexCount > 0 && instanceCount > 0)
	{
		vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
}

void DrawingCommandBuffer_Vulkan::EndRenderPass()
{
	assert(m_inRenderPass);
	vkCmdEndRenderPass(m_commandBuffer);
	m_inRenderPass = false;
}

void DrawingCommandBuffer_Vulkan::TransitionImageLayout(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, EShaderType shaderType)
{
	assert(m_isRecording);

	if (pImage->m_layout == newLayout)
	{
		return;
	}

	assert(newLayout != VK_IMAGE_LAYOUT_UNDEFINED && newLayout != VK_IMAGE_LAYOUT_PREINITIALIZED);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = pImage->m_layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Alert: Queue ownership transfer is not performed
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = pImage->m_image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (HasStencilComponent_VK(pImage->m_format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = pImage->m_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage, dstStage;

	GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, shaderType);
	GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, shaderType);

	vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); // TODO: offer a batch version

	pImage->m_layout = newLayout; // Alert: the transition may not be completed in this moment
}

void DrawingCommandBuffer_Vulkan::CopyBufferToBuffer(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, const std::shared_ptr<RawBuffer_Vulkan> pDstBuffer, const VkBufferCopy& region)
{
	assert(m_isRecording);
	vkCmdCopyBuffer(m_commandBuffer, pSrcBuffer->m_buffer, pDstBuffer->m_buffer, 1, &region); // TODO: offer a batch version
}

void DrawingCommandBuffer_Vulkan::CopyBufferToTexture2D(const std::shared_ptr<RawBuffer_Vulkan> pSrcBuffer, std::shared_ptr<Texture2D_Vulkan> pDstImage, const std::vector<VkBufferImageCopy>& regions)
{
	assert(m_isRecording);
	vkCmdCopyBufferToImage(m_commandBuffer, pSrcBuffer->m_buffer, pDstImage->m_image, pDstImage->m_layout, (uint32_t)regions.size(), regions.data());
}

void DrawingCommandBuffer_Vulkan::WaitSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_waitSemaphores.push(pSemaphore);
}

void DrawingCommandBuffer_Vulkan::SignalSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_signalSemaphores.push(pSemaphore);
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
	std::shared_ptr<DrawingCommandBuffer_Vulkan> pCommandBuffer;

	if (!m_freeCommandBuffers.TryPop(pCommandBuffer))
	{
		AllocatePrimaryCommandBuffer(1); // We can allocate more at once if more will be needed
		m_freeCommandBuffers.TryPop(pCommandBuffer);
	}
	
	m_inUseCommandBuffers.Push(pCommandBuffer);

	pCommandBuffer->BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // We only submit the command buffer once

	return pCommandBuffer;
}

void DrawingCommandManager_Vulkan::SubmitCommandBuffers(VkFence fence)
{
	std::queue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> inUseBuffers;
	m_inUseCommandBuffers.TryPopAll(inUseBuffers);

	auto pSubmitInfo = std::make_shared<CommandSubmitInfo_Vulkan>();

	while (!inUseBuffers.empty())
	{
		std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = inUseBuffers.front();

		if (ptrCopy->m_inExecution || !ptrCopy->m_isRecording)
		{
			inUseBuffers.pop();
			continue;
		}

		ptrCopy->m_isRecording = false;
		ptrCopy->m_inExecution = true;

		if (ptrCopy->m_inRenderPass)
		{
			ptrCopy->m_inRenderPass = false;
			vkCmdEndRenderPass(ptrCopy->m_commandBuffer);		
		}

		if (vkEndCommandBuffer(ptrCopy->m_commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan: couldn't end one or more command buffers.");
			continue;
		}

		pSubmitInfo->buffersAwaitSubmit.emplace_back(ptrCopy->m_commandBuffer);

		while (!ptrCopy->m_signalSemaphores.empty())
		{
			pSubmitInfo->semaphoresToSignal.emplace_back(ptrCopy->m_signalSemaphores.front()->semaphore);
			ptrCopy->m_signalSemaphores.pop();
		}

		while (!ptrCopy->m_waitSemaphores.empty())
		{
			pSubmitInfo->semaphoresToWait.emplace_back(ptrCopy->m_waitSemaphores.front()->semaphore);
			pSubmitInfo->waitStages.emplace_back(ptrCopy->m_waitSemaphores.front()->waitStage);
			ptrCopy->m_waitSemaphores.pop();
		}

		m_inExecutionCommandBuffers.Push(ptrCopy);
		inUseBuffers.pop();
	}
	
	pSubmitInfo->submitInfo = {};
	pSubmitInfo->submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	pSubmitInfo->submitInfo.commandBufferCount = (uint32_t)pSubmitInfo->buffersAwaitSubmit.size();
	pSubmitInfo->submitInfo.pCommandBuffers = pSubmitInfo->buffersAwaitSubmit.data();
	pSubmitInfo->submitInfo.signalSemaphoreCount = (uint32_t)pSubmitInfo->semaphoresToSignal.size();
	pSubmitInfo->submitInfo.pSignalSemaphores = pSubmitInfo->semaphoresToSignal.data();
	pSubmitInfo->submitInfo.waitSemaphoreCount = (uint32_t)pSubmitInfo->semaphoresToWait.size();
	pSubmitInfo->submitInfo.pWaitSemaphores = pSubmitInfo->semaphoresToWait.data();
	pSubmitInfo->submitInfo.pWaitDstStageMask = pSubmitInfo->waitStages.data();

	pSubmitInfo->fence = fence;

	m_commandSubmissionQueue.Push(pSubmitInfo);
}

void DrawingCommandManager_Vulkan::SubmitSingleCommandBuffer_Immediate(const std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer)
{
	assert(pCmdBuffer->m_isRecording);

	pCmdBuffer->m_isRecording = false;

	if (pCmdBuffer->m_inRenderPass)
	{
		pCmdBuffer->m_inRenderPass = false;
		vkCmdEndRenderPass(pCmdBuffer->m_commandBuffer);
	}

	if (vkEndCommandBuffer(pCmdBuffer->m_commandBuffer) == VK_SUCCESS)
	{
		pCmdBuffer->m_inExecution = true;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &pCmdBuffer->m_commandBuffer;
		submitInfo.pWaitDstStageMask = 0;

		vkQueueSubmit(m_workingQueue.queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_workingQueue.queue);

		pCmdBuffer->m_inExecution = false;
		m_freeCommandBuffers.Push(pCmdBuffer);
	}
	else
	{
		throw std::runtime_error("Vulkan: couldn't end a command buffer.");
	}
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
	std::shared_ptr<CommandSubmitInfo_Vulkan> pCommandSubmitInfo;
	bool shouldRecycle = false;

	while (m_isRunning)
	{
		shouldRecycle = false;
		while (m_commandSubmissionQueue.TryPop(pCommandSubmitInfo))
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
	std::unordered_map<std::shared_ptr<DrawingFence_Vulkan>, std::vector<std::shared_ptr<DrawingCommandBuffer_Vulkan>>> fenceMappings;

	lock.lock(); // If the mutex object is not initially locked when waited, it would cause undefined behavior

	bool recycleFlag = false;
	while (m_isRunning)
	{
		m_commandBufferRecycleFlag.GetValue(recycleFlag);
		if (!recycleFlag) // Check if there is any recycle notification during last recycle
		{
			m_commandBufferRecycleCv.wait(lock);
		}
		m_commandBufferRecycleFlag.AssignValue(false);

		std::queue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> inExecutionCmdBufferQueue;
		m_inExecutionCommandBuffers.TryPopAll(inExecutionCmdBufferQueue);
		while (!inExecutionCmdBufferQueue.empty())
		{
			std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = inExecutionCmdBufferQueue.front();
			assert(ptrCopy->m_pAssociatedFence);
			fenceMappings[ptrCopy->m_pAssociatedFence].emplace_back(ptrCopy);				
			inExecutionCmdBufferQueue.pop();
		}

		for (auto& fenceMap : fenceMappings)
		{
			if (fenceMap.second.size() > 0)
			{
				VkFence fenceToQuery = fenceMap.first->fence;
				if (vkWaitForFences(m_pDevice->logicalDevice, 1, &fenceToQuery, VK_FALSE, RECYCLE_TIMEOUT) == VK_SUCCESS)
				{
					for (auto pCmdBuffer : fenceMap.second)
					{
						pCmdBuffer->m_pAssociatedFence = nullptr;
						pCmdBuffer->m_inExecution = false;
						m_freeCommandBuffers.Push(pCmdBuffer);
					}
					m_pDevice->pSyncObjectManager->ReturnFence(fenceMap.first);
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