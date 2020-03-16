#include "DrawingCommandManager_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>

using namespace Engine;

DrawingCommandBuffer_Vulkan::DrawingCommandBuffer_Vulkan(const VkCommandBuffer& cmdBuffer)
	: m_isRecording(false), m_inRenderPass(false), m_inExecution(false), m_pAssociatedSubmitSemaphore(nullptr), m_commandBuffer(cmdBuffer), m_pipelineLayout(VK_NULL_HANDLE), 
	m_pSyncObjectManager(nullptr), m_isExternal(false), m_usageFlags(0)
{

}

DrawingCommandBuffer_Vulkan::~DrawingCommandBuffer_Vulkan()
{
	if (!m_inExecution)
	{
		assert(m_pSyncObjectManager != nullptr);

		// Alert: freeing the semaphores here may result in conflicts
		for (auto& pSemaphore : m_waitPresentationSemaphores)
		{
			m_pSyncObjectManager->ReturnSemaphore(pSemaphore);
		}
		for (auto& pSemaphore : m_signalPresentationSemaphores)
		{
			m_pSyncObjectManager->ReturnSemaphore(pSemaphore);
		}
		for (auto& pSemaphore : m_waitSemaphores)
		{
			m_pSyncObjectManager->ReturnTimelineSemaphore(pSemaphore);
		}
		for (auto& pSemaphore : m_signalSemaphores)
		{
			m_pSyncObjectManager->ReturnTimelineSemaphore(pSemaphore);
		}
	}
	else
	{
		throw std::runtime_error("Vulkan: shouldn't destroy command buffer that is in execution.");
	}
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

void DrawingCommandBuffer_Vulkan::BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pVertexBuffers, const VkDeviceSize* pOffsets)
{
	assert(m_isRecording);
	vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, pVertexBuffers, pOffsets);
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
	passBeginInfo.framebuffer = frameBuffer;
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

void DrawingCommandBuffer_Vulkan::BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<std::shared_ptr<DrawingDescriptorSet_Vulkan>>& descriptorSets, uint32_t firstSet)
{
	assert(m_isRecording);

	std::vector<VkDescriptorSet> setHandles;
	for (auto& pSet : descriptorSets)
	{
		m_boundDescriptorSets.push(pSet);
		setHandles.emplace_back(pSet->m_descriptorSet);
	}

	vkCmdBindDescriptorSets(m_commandBuffer, bindPoint, m_pipelineLayout, firstSet, (uint32_t)setHandles.size(), setHandles.data(), 0, nullptr);
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

void DrawingCommandBuffer_Vulkan::DrawPrimitive(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	assert(m_inRenderPass);
	if (vertexCount > 0 && instanceCount > 0)
	{
		vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
}

void DrawingCommandBuffer_Vulkan::EndRenderPass()
{
	assert(m_inRenderPass);
	vkCmdEndRenderPass(m_commandBuffer);
	m_inRenderPass = false;
}

void DrawingCommandBuffer_Vulkan::EndCommandBuffer()
{
	if (m_inRenderPass)
	{
		m_inRenderPass = false;
		vkCmdEndRenderPass(m_commandBuffer);
	}

	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: couldn't end one or more command buffers.");
		return;
	}
}

void DrawingCommandBuffer_Vulkan::TransitionImageLayout(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, uint32_t appliedStages)
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
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Queue ownership transfer is not performed
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = pImage->m_image;

	// Determine new aspect mask
	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (HasStencilComponent_VK(pImage->m_format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		break;
	}
	case VK_IMAGE_LAYOUT_GENERAL:
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	}
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	}
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
	{
		barrier.subresourceRange.aspectMask = pImage->m_aspect;
		break;
	}
	default:
		std::cerr << "Vulkan: Unhandled image layout: " << newLayout << std::endl;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = pImage->m_mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage, dstStage;

	GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
	GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, appliedStages);

	vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); // TODO: offer a batch version

	pImage->m_layout = newLayout; // Alert: the transition may not be completed in this moment
	pImage->m_appliedStages = appliedStages;
}

void DrawingCommandBuffer_Vulkan::GenerateMipmap(std::shared_ptr<Texture2D_Vulkan> pImage, const VkImageLayout newLayout, uint32_t appliedStages)
{
#if defined(_DEBUG)
	// Check whether linear blitting on given image's format is supported
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(pImage->m_pDevice->physicalDevice, pImage->m_format, &formatProperties);
	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
#endif

	assert(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = pImage->m_image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = pImage->m_aspect;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	uint32_t mipWidth = pImage->m_width;
	uint32_t mipHeight = pImage->m_height;

	for (uint32_t i = 1; i < pImage->m_mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = pImage->m_layout;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkPipelineStageFlags srcStage, dstStage;

		GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
		GetAccessAndStageFromImageLayout_VK(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, barrier.dstAccessMask, dstStage, 0);

		vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blitRegion = {};
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = { (int32_t)mipWidth, (int32_t)mipHeight, 1 };
		blitRegion.srcSubresource.aspectMask = pImage->m_aspect;
		blitRegion.srcSubresource.mipLevel = i - 1;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = { mipWidth > 1 ? (int32_t)mipWidth / 2 : 1, mipHeight > 1 ? (int32_t)mipHeight / 2 : 1, 1 };
		blitRegion.dstSubresource.aspectMask = pImage->m_aspect;
		blitRegion.dstSubresource.mipLevel = i;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;

		vkCmdBlitImage(m_commandBuffer, 
			pImage->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			pImage->m_image, pImage->m_layout, 
			1, &blitRegion, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = newLayout;

		GetAccessAndStageFromImageLayout_VK(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, barrier.srcAccessMask, srcStage, 0);
		GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, appliedStages);

		vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1)
		{
			mipWidth /= 2;
		}
		if (mipHeight > 1)
		{
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = pImage->m_mipLevels - 1;
	barrier.oldLayout = pImage->m_layout;
	barrier.newLayout = newLayout;

	VkPipelineStageFlags srcStage, dstStage;

	GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
	GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, appliedStages);

	vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	pImage->m_layout = newLayout; // Alert: the transition may not be completed in this moment
	pImage->m_appliedStages = appliedStages;
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

void DrawingCommandBuffer_Vulkan::CopyTexture2DToBuffer(std::shared_ptr<Texture2D_Vulkan> pSrcImage, const std::shared_ptr<RawBuffer_Vulkan> pDstBuffer, const std::vector<VkBufferImageCopy>& regions)
{
	assert(m_isRecording);
	vkCmdCopyImageToBuffer(m_commandBuffer, pSrcImage->m_image, pSrcImage->m_layout, pDstBuffer->m_buffer, (uint32_t)regions.size(), regions.data());
}

void DrawingCommandBuffer_Vulkan::WaitPresentationSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_waitPresentationSemaphores.emplace_back(pSemaphore);
}

void DrawingCommandBuffer_Vulkan::SignalPresentationSemaphore(const std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore)
{
	m_signalPresentationSemaphores.emplace_back(pSemaphore);
}

void DrawingCommandBuffer_Vulkan::WaitSemaphore(const std::shared_ptr<TimelineSemaphore_Vulkan> pSemaphore)
{
	m_waitSemaphores.emplace_back(pSemaphore);
}

void DrawingCommandBuffer_Vulkan::SignalSemaphore(const std::shared_ptr<TimelineSemaphore_Vulkan> pSemaphore)
{
	m_signalSemaphores.emplace_back(pSemaphore);
}

DrawingCommandPool_Vulkan::DrawingCommandPool_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkCommandPool poolHandle, DrawingCommandManager_Vulkan* pManager)
	: m_pDevice(pDevice), m_commandPool(poolHandle), m_allocatedCommandBufferCount(0), m_pManager(pManager)
{

}

DrawingCommandPool_Vulkan::~DrawingCommandPool_Vulkan()
{
	vkDestroyCommandPool(m_pDevice->logicalDevice, m_commandPool, nullptr);
}

std::shared_ptr<DrawingCommandBuffer_Vulkan> DrawingCommandPool_Vulkan::RequestPrimaryCommandBuffer()
{
	std::shared_ptr<DrawingCommandBuffer_Vulkan> pCommandBuffer = nullptr;

	if (!m_freeCommandBuffers.TryPop(pCommandBuffer))
	{
		AllocatePrimaryCommandBuffer(1); // We can allocate more at once if more will be needed
		m_freeCommandBuffers.TryPop(pCommandBuffer);
	}

	pCommandBuffer->m_pAllocatedPool = this;
	pCommandBuffer->BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return pCommandBuffer;
}

bool DrawingCommandPool_Vulkan::AllocatePrimaryCommandBuffer(uint32_t count)
{
	assert(m_allocatedCommandBufferCount + count <= MAX_COMMAND_BUFFER_COUNT);

	std::vector<VkCommandBuffer> cmdBufferHandles(count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	if (vkAllocateCommandBuffers(m_pDevice->logicalDevice, &allocInfo, cmdBufferHandles.data()) == VK_SUCCESS)
	{
		m_allocatedCommandBufferCount += count;

		for (auto& cmdBuffer : cmdBufferHandles)
		{
			auto pNewCmdBuffer = std::make_shared<DrawingCommandBuffer_Vulkan>(cmdBuffer);
			pNewCmdBuffer->m_pSyncObjectManager = m_pDevice->pSyncObjectManager;

			m_freeCommandBuffers.Push(pNewCmdBuffer);
		}
		return true;
	}

	throw std::runtime_error("Vulkan: Failed to allocate command buffer.");
	return false;
}

DrawingCommandManager_Vulkan::DrawingCommandManager_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingCommandQueue_Vulkan& queue)
	: m_pDevice(pDevice), m_workingQueue(queue)
{
	m_pDefaultCommandPool = std::make_shared<DrawingCommandPool_Vulkan>(m_pDevice, CreateCommandPool(), this);

	m_isRunning = true;

	m_commandBufferSubmissionThread = std::thread(&DrawingCommandManager_Vulkan::SubmitCommandBufferAsync, this);
	m_commandBufferRecycleThread = std::thread(&DrawingCommandManager_Vulkan::RecycleCommandBufferAsync, this);
}

DrawingCommandManager_Vulkan::~DrawingCommandManager_Vulkan()
{
	if (m_isRunning)
	{
		Destroy();
	}
}

void DrawingCommandManager_Vulkan::Destroy()
{
	m_isRunning = false;

	m_commandBufferSubmissionThread.join();
	m_commandBufferRecycleThread.join();
}

EQueueType DrawingCommandManager_Vulkan::GetWorkingQueueType() const
{
	return m_workingQueue.type;
}

std::shared_ptr<DrawingCommandBuffer_Vulkan> DrawingCommandManager_Vulkan::RequestPrimaryCommandBuffer()
{
	std::shared_ptr<DrawingCommandBuffer_Vulkan> pCommandBuffer = m_pDefaultCommandPool->RequestPrimaryCommandBuffer();
	pCommandBuffer->m_usageFlags = (uint32_t)EDrawingCommandBufferUsageFlagBits_Vulkan::Implicit;
	m_inUseCommandBuffers.Push(pCommandBuffer);

	return pCommandBuffer;
}

void DrawingCommandManager_Vulkan::SubmitCommandBuffers(std::shared_ptr <TimelineSemaphore_Vulkan> pSubmitSemaphore, uint32_t usageMask)
{
	assert(pSubmitSemaphore != nullptr);

	std::queue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> inUseBuffers;
	m_inUseCommandBuffers.TryPopAll(inUseBuffers);

	auto pSubmitInfo = std::make_shared<CommandSubmitInfo_Vulkan>();

	while (!inUseBuffers.empty())
	{
		std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = inUseBuffers.front();

		if ((ptrCopy->m_usageFlags & usageMask) == 0)
		{
			m_inUseCommandBuffers.Push(ptrCopy);
			inUseBuffers.pop();
			continue;
		}

		if (ptrCopy->m_inExecution || !ptrCopy->m_isRecording) // Already submitted through SubmitSingleCommandBuffer_Immediate
		{
			inUseBuffers.pop();
			continue;
		}

		ptrCopy->m_isRecording = false;
		ptrCopy->m_inExecution = true;

		if (!ptrCopy->m_isExternal) // External command buffers should only be ended by their host thread
		{
			ptrCopy->EndCommandBuffer(); // TODO: check for command buffer threading error here
		}

		pSubmitInfo->buffersAwaitSubmit.emplace_back(ptrCopy->m_commandBuffer);
		pSubmitInfo->queuedCmdBuffers.push(ptrCopy);

		for (auto& pSemaphore : ptrCopy->m_signalSemaphores)
		{
			pSubmitInfo->semaphoresToSignal.emplace_back(pSemaphore->semaphore);
			pSubmitInfo->signalSemaphoreValues.emplace_back(pSemaphore->m_signalValue);
		}
		for (auto& pSemaphore : ptrCopy->m_waitSemaphores)
		{
			pSubmitInfo->semaphoresToWait.emplace_back(pSemaphore->semaphore);
			pSubmitInfo->waitStages.emplace_back(pSemaphore->waitStage);
			pSubmitInfo->waitSemaphoreValues.emplace_back(pSemaphore->m_waitValue);
		}

		for (auto& pSemaphore : ptrCopy->m_signalPresentationSemaphores)
		{
			pSubmitInfo->semaphoresToSignal.emplace_back(pSemaphore->semaphore);
			pSubmitInfo->signalSemaphoreValues.emplace_back(0); // This value should be ignored by driver implementation
		}
		for (auto & pSemaphore : ptrCopy->m_waitPresentationSemaphores)
		{
			pSubmitInfo->semaphoresToWait.emplace_back(pSemaphore->semaphore);
			pSubmitInfo->waitStages.emplace_back(pSemaphore->waitStage);
			pSubmitInfo->waitSemaphoreValues.emplace_back(0); // This value should be ignored by driver implementation
		}

		ptrCopy->m_pAssociatedSubmitSemaphore = pSubmitSemaphore;

		inUseBuffers.pop();
	}
	
	if (pSubmitInfo->buffersAwaitSubmit.size() == 0)
	{
		m_pDevice->pSyncObjectManager->ReturnTimelineSemaphore(pSubmitSemaphore);
		return;
	}

	pSubmitInfo->semaphoresToSignal.emplace_back(pSubmitSemaphore->semaphore);
	pSubmitInfo->signalSemaphoreValues.emplace_back(pSubmitSemaphore->m_signalValue);

	pSubmitInfo->timelineSemaphoreSubmitInfo = {};
	pSubmitInfo->timelineSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	pSubmitInfo->timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = (uint32_t)pSubmitInfo->signalSemaphoreValues.size();
	pSubmitInfo->timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = pSubmitInfo->signalSemaphoreValues.data();
	pSubmitInfo->timelineSemaphoreSubmitInfo.waitSemaphoreValueCount = (uint32_t)pSubmitInfo->waitSemaphoreValues.size();
	pSubmitInfo->timelineSemaphoreSubmitInfo.pWaitSemaphoreValues = pSubmitInfo->waitSemaphoreValues.data();

	pSubmitInfo->submitInfo = {};
	pSubmitInfo->submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	pSubmitInfo->submitInfo.pNext = &pSubmitInfo->timelineSemaphoreSubmitInfo;
	pSubmitInfo->submitInfo.commandBufferCount = (uint32_t)pSubmitInfo->buffersAwaitSubmit.size();
	pSubmitInfo->submitInfo.pCommandBuffers = pSubmitInfo->buffersAwaitSubmit.data();
	pSubmitInfo->submitInfo.signalSemaphoreCount = (uint32_t)pSubmitInfo->semaphoresToSignal.size();
	pSubmitInfo->submitInfo.pSignalSemaphores = pSubmitInfo->semaphoresToSignal.data();
	pSubmitInfo->submitInfo.waitSemaphoreCount = (uint32_t)pSubmitInfo->semaphoresToWait.size();
	pSubmitInfo->submitInfo.pWaitSemaphores = pSubmitInfo->semaphoresToWait.data();
	pSubmitInfo->submitInfo.pWaitDstStageMask = pSubmitInfo->waitStages.data();

	m_commandSubmissionQueue.Push(pSubmitInfo);

	{
		std::lock_guard<std::mutex> guard(m_commandBufferSubmissionMutex);
		m_commandBufferSubmissionFlag = true;
	}
	m_commandBufferSubmissionCv.notify_one();
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
		// TODO: handle possible semaphore submission

		vkQueueSubmit(m_workingQueue.queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_workingQueue.queue);

		pCmdBuffer->m_inExecution = false;
		m_pDefaultCommandPool->m_freeCommandBuffers.Push(pCmdBuffer);
	}
	else
	{
		throw std::runtime_error("Vulkan: couldn't end a command buffer.");
	}
}

std::shared_ptr<DrawingCommandPool_Vulkan> DrawingCommandManager_Vulkan::RequestExternalCommandPool()
{
	std::lock_guard<std::mutex> lock(m_externalCommandPoolCreationMutex);
	return std::make_shared<DrawingCommandPool_Vulkan>(m_pDevice, CreateCommandPool(), this);
}

void DrawingCommandManager_Vulkan::ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer)
{
	m_inUseCommandBuffers.Push(pCmdBuffer);
}

VkCommandPool DrawingCommandManager_Vulkan::CreateCommandPool()
{
	VkCommandPool newPool = VK_NULL_HANDLE;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_workingQueue.queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_pDevice->logicalDevice, &poolInfo, nullptr, &newPool) == VK_SUCCESS)
	{
		return newPool;
	}
	else
	{
		throw std::runtime_error("Vulkan: Failed to create command pool.");
		return VK_NULL_HANDLE;
	}
}

void DrawingCommandManager_Vulkan::SubmitCommandBufferAsync()
{
	std::shared_ptr<CommandSubmitInfo_Vulkan> pCommandSubmitInfo;

	while (m_isRunning)
	{
		{
			std::unique_lock<std::mutex> lock(m_commandBufferSubmissionMutex);
			m_commandBufferSubmissionCv.wait(lock, [this]() { return m_commandBufferSubmissionFlag == true; });
			m_commandBufferSubmissionFlag = false;
		}

		while (m_commandSubmissionQueue.TryPop(pCommandSubmitInfo))
		{
			vkQueueSubmit(m_workingQueue.queue, 1, &pCommandSubmitInfo->submitInfo, VK_NULL_HANDLE);			

			{
				std::lock_guard<std::mutex> guard(m_inExecutionQueueRWMutex);

				while (!pCommandSubmitInfo->queuedCmdBuffers.empty())
				{
					std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = pCommandSubmitInfo->queuedCmdBuffers.front();
					m_inExecutionCommandBuffers.Push(ptrCopy);
					pCommandSubmitInfo->queuedCmdBuffers.pop();
				}
			}

			{
				std::lock_guard<std::mutex> guard(m_commandBufferRecycleMutex);
				m_commandBufferRecycleFlag = true;
			}
			m_commandBufferRecycleCv.notify_one();

			std::this_thread::yield();
		}
	}
}

void DrawingCommandManager_Vulkan::RecycleCommandBufferAsync()
{
	std::unordered_map<std::shared_ptr<TimelineSemaphore_Vulkan>, std::vector<std::shared_ptr<DrawingCommandBuffer_Vulkan>>> timelineGroups;

	while (m_isRunning)
	{
		{
			std::unique_lock<std::mutex> lock(m_commandBufferRecycleMutex);
			m_commandBufferRecycleCv.wait(lock, [this]() { return m_commandBufferRecycleFlag == true; });
			m_commandBufferRecycleFlag = false;
		}

		std::queue<std::shared_ptr<DrawingCommandBuffer_Vulkan>> inExecutionCmdBufferQueue;

		{
			std::lock_guard<std::mutex> guard(m_inExecutionQueueRWMutex);
			m_inExecutionCommandBuffers.TryPopAll(inExecutionCmdBufferQueue);
		}
		while (!inExecutionCmdBufferQueue.empty()) // Group command buffers by timeline semaphore
		{
			std::shared_ptr<DrawingCommandBuffer_Vulkan> ptrCopy = inExecutionCmdBufferQueue.front();
			assert(ptrCopy->m_pAssociatedSubmitSemaphore);
			timelineGroups[ptrCopy->m_pAssociatedSubmitSemaphore].emplace_back(ptrCopy);
			inExecutionCmdBufferQueue.pop();
		}

		for (auto& timelineGroup : timelineGroups)
		{
			if (timelineGroup.second.size() > 0)
			{
				auto pSemaphoreToWait = timelineGroup.first;

				if (pSemaphoreToWait->Wait(RECYCLE_TIMEOUT) == VK_SUCCESS)
				{
					for (auto& pCmdBuffer : timelineGroup.second)
					{
						for (auto& pSemaphore : pCmdBuffer->m_waitPresentationSemaphores)
						{
							m_pDevice->pSyncObjectManager->ReturnSemaphore(pSemaphore);
						}
						for (auto& pSemaphore : pCmdBuffer->m_waitSemaphores)
						{
							m_pDevice->pSyncObjectManager->ReturnTimelineSemaphore(pSemaphore);
						}
						pCmdBuffer->m_waitPresentationSemaphores.clear();
						pCmdBuffer->m_signalPresentationSemaphores.clear();
						pCmdBuffer->m_waitSemaphores.clear();
						pCmdBuffer->m_signalSemaphores.clear();

						while (!pCmdBuffer->m_boundDescriptorSets.empty())
						{
							pCmdBuffer->m_boundDescriptorSets.front()->m_isInUse = false;
							pCmdBuffer->m_boundDescriptorSets.pop();
						}

						pCmdBuffer->m_pAssociatedSubmitSemaphore = nullptr;
						pCmdBuffer->m_inExecution = false;

						pCmdBuffer->m_pAllocatedPool->m_freeCommandBuffers.Push(pCmdBuffer); // Alert: if the pool was destroyed when recycling, this would cause violation
					}

					m_pDevice->pSyncObjectManager->ReturnTimelineSemaphore(pSemaphoreToWait);
				}
				else
				{
					for (auto& pCmdBuffer : timelineGroup.second)
					{
						std::cerr << "Vulkan: Command buffer " << pCmdBuffer->m_commandBuffer << " exection timeout. Usage flag is " 
							<< pCmdBuffer->m_usageFlags << ". DebugID is " << pCmdBuffer->m_debugID  << "." << std::endl;
					}
					throw std::runtime_error("Vulkan: command execution timeout.");

					m_pDevice->pSyncObjectManager->ReturnTimelineSemaphore(pSemaphoreToWait);
				}
				timelineGroup.second.clear();
			}
		}

		std::this_thread::yield();
	}

	timelineGroups.clear();
}