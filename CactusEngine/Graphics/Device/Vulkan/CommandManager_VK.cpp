#include "CommandManager_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "Buffers_VK.h"
#include "Textures_VK.h"
#include "GHIUtilities_VK.h"
#include "LogUtility.h"
#include "MemoryAllocator.h"

#include <thread>
#include <mutex>
#include <condition_variable>

namespace Engine
{
	CommandBuffer_VK::CommandBuffer_VK(const VkCommandBuffer& cmdBuffer)
		: m_isRecording(false),
		m_inRenderPass(false),
		m_inExecution(false),
		m_pAssociatedSubmitSemaphore(nullptr),
		m_commandBuffer(cmdBuffer),
		m_pipelineLayout(VK_NULL_HANDLE),
		m_pSyncObjectManager(nullptr),
		m_isExternal(false),
		m_pAllocatedPool(nullptr),
		m_usageFlags(0)
	{

	}

	CommandBuffer_VK::~CommandBuffer_VK()
	{
		if (!m_inExecution)
		{
			DEBUG_ASSERT_CE(m_pSyncObjectManager != nullptr);

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

	bool CommandBuffer_VK::IsRecording() const
	{
		return m_isRecording;
	}

	bool CommandBuffer_VK::InRenderPass() const
	{
		return m_inRenderPass;
	}

	bool CommandBuffer_VK::InExecution() const
	{
		return m_inExecution;
	}

	void CommandBuffer_VK::BeginCommandBuffer(VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = usage;

		VkResult result = vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
		DEBUG_ASSERT_CE(result == VK_SUCCESS);

		m_isRecording = true;
		m_inRenderPass = false;
		m_inExecution = false;
	}

	void CommandBuffer_VK::BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pVertexBuffers, const VkDeviceSize* pOffsets)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, pVertexBuffers, pOffsets);
	}

	void CommandBuffer_VK::BindIndexBuffer(const VkBuffer indexBuffer, const VkDeviceSize offset, VkIndexType type)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer, offset, type);
	}

	void CommandBuffer_VK::BeginRenderPass(const VkRenderPass renderPass, const VkFramebuffer frameBuffer, const std::vector<VkClearValue>& clearValues, const VkExtent2D& areaExtent, const VkOffset2D& areaOffset)
	{
		DEBUG_ASSERT_CE(m_isRecording);

		VkRenderPassBeginInfo passBeginInfo{};
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

	void CommandBuffer_VK::BindPipeline(const VkPipelineBindPoint bindPoint, const VkPipeline pipeline)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdBindPipeline(m_commandBuffer, bindPoint, pipeline);
	}

	void CommandBuffer_VK::BindPipelineLayout(const VkPipelineLayout pipelineLayout)
	{
		m_pipelineLayout = pipelineLayout;
	}

	void CommandBuffer_VK::BindDescriptorSets(const VkPipelineBindPoint bindPoint, const std::vector<DescriptorSet_VK*>& descriptorSets, uint32_t firstSet)
	{
		DEBUG_ASSERT_CE(m_isRecording);

		std::vector<VkDescriptorSet> setHandles;
		for (auto& pSet : descriptorSets)
		{
			m_boundDescriptorSets.push(pSet);
			setHandles.emplace_back(pSet->m_descriptorSet);
		}

		vkCmdBindDescriptorSets(m_commandBuffer, bindPoint, m_pipelineLayout, firstSet, (uint32_t)setHandles.size(), setHandles.data(), 0, nullptr);
		// Alert: dynamic offset is not handled
	}

	void CommandBuffer_VK::SetViewport(const VkViewport* pViewport, const VkRect2D* pScissor)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		DEBUG_ASSERT_CE(pViewport != nullptr);
		DEBUG_ASSERT_CE(pScissor != nullptr);

		vkCmdSetViewport(m_commandBuffer, 0, 1, pViewport);
		vkCmdSetScissor(m_commandBuffer, 0, 1, pScissor);
	}

	void CommandBuffer_VK::DrawPrimitiveIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		DEBUG_ASSERT_CE(m_inRenderPass);
		if (indexCount > 0 && instanceCount > 0)
		{
			vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
	}

	void CommandBuffer_VK::DrawPrimitive(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		DEBUG_ASSERT_CE(m_inRenderPass);
		if (vertexCount > 0 && instanceCount > 0)
		{
			vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
		}
	}

	void CommandBuffer_VK::EndRenderPass()
	{
		DEBUG_ASSERT_CE(m_inRenderPass);
		vkCmdEndRenderPass(m_commandBuffer);
		m_inRenderPass = false;
	}

	void CommandBuffer_VK::EndCommandBuffer()
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

	void CommandBuffer_VK::TransitionImageLayout(Texture2D_VK* pImage, const VkImageLayout newLayout, uint32_t appliedStages)
	{
		DEBUG_ASSERT_CE(m_isRecording);

		if (pImage->m_layout == newLayout)
		{
			return;
		}

		DEBUG_ASSERT_CE(newLayout != VK_IMAGE_LAYOUT_UNDEFINED && newLayout != VK_IMAGE_LAYOUT_PREINITIALIZED);

		VkImageMemoryBarrier barrier{};
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
			LOG_ERROR("Vulkan: Unhandled image layout: " + std::to_string((uint32_t)newLayout));
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = pImage->m_mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags srcStage = 0, dstStage = 0;

		GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
		GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, appliedStages);
		vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); // TODO: offer a batch version

		pImage->m_layout = newLayout; // Alert: the transition may not be completed in this moment
		pImage->m_appliedStages = appliedStages;
	}

	void CommandBuffer_VK::GenerateMipmap(Texture2D_VK* pImage, const VkImageLayout newLayout, uint32_t appliedStages)
	{
#if defined(DEBUG_MODE_CE)
		// Check whether linear blitting on given image's format is supported
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(pImage->m_pDevice->physicalDevice, pImage->m_format, &formatProperties);
		DEBUG_ASSERT_CE(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
#endif
		DEBUG_ASSERT_CE(m_isRecording);
		DEBUG_ASSERT_CE(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

		VkImageMemoryBarrier barrier{};
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

			VkPipelineStageFlags srcStage = 0, dstStage = 0;

			GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
			GetAccessAndStageFromImageLayout_VK(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, barrier.dstAccessMask, dstStage, 0);
			vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			VkImageBlit blitRegion{};
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

		VkPipelineStageFlags srcStage = 0, dstStage = 0;

		GetAccessAndStageFromImageLayout_VK(pImage->m_layout, barrier.srcAccessMask, srcStage, pImage->m_appliedStages);
		GetAccessAndStageFromImageLayout_VK(newLayout, barrier.dstAccessMask, dstStage, appliedStages);
		vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		pImage->m_layout = newLayout; // Alert: the transition may not be completed in this moment
		pImage->m_appliedStages = appliedStages;
	}

	void CommandBuffer_VK::CopyBufferToBuffer(const RawBuffer_VK* pSrcBuffer, const RawBuffer_VK* pDstBuffer, const VkBufferCopy& region)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdCopyBuffer(m_commandBuffer, pSrcBuffer->m_buffer, pDstBuffer->m_buffer, 1, &region); // TODO: offer a batch version
	}

	void CommandBuffer_VK::CopyBufferToTexture2D(const RawBuffer_VK* pSrcBuffer, Texture2D_VK* pDstImage, const std::vector<VkBufferImageCopy>& regions)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdCopyBufferToImage(m_commandBuffer, pSrcBuffer->m_buffer, pDstImage->m_image, pDstImage->m_layout, (uint32_t)regions.size(), regions.data());
	}

	void CommandBuffer_VK::CopyTexture2DToBuffer(Texture2D_VK* pSrcImage, const RawBuffer_VK* pDstBuffer, const std::vector<VkBufferImageCopy>& regions)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdCopyImageToBuffer(m_commandBuffer, pSrcImage->m_image, pSrcImage->m_layout, pDstBuffer->m_buffer, (uint32_t)regions.size(), regions.data());
	}

	void CommandBuffer_VK::CopyTexture2DToTexture2D(Texture2D_VK* pSrcImage, Texture2D_VK* pDstImage, const std::vector<VkImageCopy>& regions)
	{
		DEBUG_ASSERT_CE(m_isRecording);
		vkCmdCopyImage(m_commandBuffer, pSrcImage->m_image, pSrcImage->m_layout, pDstImage->m_image, pDstImage->m_layout, (uint32_t)regions.size(), regions.data());
	}

	void CommandBuffer_VK::WaitPresentationSemaphore(Semaphore_VK* pSemaphore)
	{
		m_waitPresentationSemaphores.emplace_back(pSemaphore);
	}

	void CommandBuffer_VK::SignalPresentationSemaphore(Semaphore_VK* pSemaphore)
	{
		m_signalPresentationSemaphores.emplace_back(pSemaphore);
	}

	void CommandBuffer_VK::WaitSemaphore(TimelineSemaphore_VK* pSemaphore)
	{
		m_waitSemaphores.emplace_back(pSemaphore);
	}

	void CommandBuffer_VK::SignalSemaphore(TimelineSemaphore_VK* pSemaphore)
	{
		m_signalSemaphores.emplace_back(pSemaphore);
	}

	CommandPool_VK::CommandPool_VK(LogicalDevice_VK* pDevice, VkCommandPool poolHandle, CommandManager_VK* pManager)
		: m_pDevice(pDevice),
		m_commandPool(poolHandle),
		m_allocatedCommandBufferCount(0),
		m_pManager(pManager)
	{

	}

	CommandPool_VK::~CommandPool_VK()
	{
		vkDestroyCommandPool(m_pDevice->logicalDevice, m_commandPool, nullptr);
	}

	CommandBuffer_VK* CommandPool_VK::RequestPrimaryCommandBuffer()
	{
		CommandBuffer_VK* pCommandBuffer = nullptr;

		if (!m_freeCommandBuffers.TryPop(pCommandBuffer))
		{
			AllocatePrimaryCommandBuffer(1); // We can allocate more at once if more will be needed
			m_freeCommandBuffers.TryPop(pCommandBuffer);
		}

		pCommandBuffer->m_pAllocatedPool = this;
		pCommandBuffer->BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		return pCommandBuffer;
	}

	bool CommandPool_VK::AllocatePrimaryCommandBuffer(uint32_t count)
	{
		DEBUG_ASSERT_CE(m_allocatedCommandBufferCount + count <= MAX_COMMAND_BUFFER_COUNT);

		std::vector<VkCommandBuffer> cmdBufferHandles(count);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = count;

		if (vkAllocateCommandBuffers(m_pDevice->logicalDevice, &allocInfo, cmdBufferHandles.data()) == VK_SUCCESS)
		{
			m_allocatedCommandBufferCount += count;

			for (auto& cmdBuffer : cmdBufferHandles)
			{
				CommandBuffer_VK* pNewCmdBuffer;
				CE_NEW(pNewCmdBuffer, CommandBuffer_VK, cmdBuffer);
				pNewCmdBuffer->m_pSyncObjectManager = m_pDevice->pSyncObjectManager;

				m_freeCommandBuffers.Push(pNewCmdBuffer);
			}
			return true;
		}

		throw std::runtime_error("Vulkan: Failed to allocate command buffer.");
		return false;
	}

	CommandManager_VK::CommandManager_VK(LogicalDevice_VK* pDevice, const CommandQueue_VK& queue)
		: m_pDevice(pDevice),
		m_workingQueue(queue)
	{
		CE_NEW(m_pDefaultCommandPool, CommandPool_VK, m_pDevice, CreateCommandPool(), this);

		m_isRunning = true;

		m_commandBufferSubmissionThread = std::thread(&CommandManager_VK::SubmitCommandBufferAsync, this);
		m_commandBufferRecycleThread = std::thread(&CommandManager_VK::RecycleCommandBufferAsync, this);
	}

	CommandManager_VK::~CommandManager_VK()
	{
		if (m_isRunning)
		{
			Destroy();
		}
	}

	void CommandManager_VK::Destroy()
	{
		m_isRunning = false;

		m_commandBufferSubmissionThread.join();
		m_commandBufferRecycleThread.join();
	}

	EQueueType CommandManager_VK::GetWorkingQueueType() const
	{
		return m_workingQueue.type;
	}

	void CommandManager_VK::WaitWorkingQueueIdle()
	{
		vkQueueWaitIdle(m_workingQueue.queue);
	}

	CommandBuffer_VK* CommandManager_VK::RequestPrimaryCommandBuffer()
	{
		CommandBuffer_VK* pCommandBuffer = m_pDefaultCommandPool->RequestPrimaryCommandBuffer();
		pCommandBuffer->m_usageFlags = (uint32_t)ECommandBufferUsageFlagBits_VK::Implicit;
		m_inUseCommandBuffers.Push(pCommandBuffer);

		return pCommandBuffer;
	}

	void CommandManager_VK::SubmitCommandBuffers(TimelineSemaphore_VK* pSubmitSemaphore, uint32_t usageMask, ThreadSemaphore* pNotifySemaphore)
	{
		DEBUG_ASSERT_CE(pSubmitSemaphore != nullptr);

		std::queue<CommandBuffer_VK*> inUseBuffers;
		m_inUseCommandBuffers.TryPopAll(inUseBuffers);

		CommandSubmitInfo_VK* pSubmitInfo;
		CE_NEW(pSubmitInfo, CommandSubmitInfo_VK);

		while (!inUseBuffers.empty())
		{
			CommandBuffer_VK* ptrCopy = inUseBuffers.front();

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
			for (auto& pSemaphore : ptrCopy->m_waitPresentationSemaphores)
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

		pSubmitInfo->pNotifySemaphore = pNotifySemaphore;

		m_commandSubmissionQueue.Push(pSubmitInfo);

		{
			std::lock_guard<std::mutex> guard(m_commandBufferSubmissionMutex);
			m_commandBufferSubmissionFlag = true;
		}
		m_commandBufferSubmissionCv.notify_one();
	}

	void CommandManager_VK::SubmitSingleCommandBuffer_Immediate(CommandBuffer_VK* pCmdBuffer)
	{
		DEBUG_ASSERT_CE(pCmdBuffer->m_isRecording);

		pCmdBuffer->m_isRecording = false;

		if (pCmdBuffer->m_inRenderPass)
		{
			pCmdBuffer->m_inRenderPass = false;
			vkCmdEndRenderPass(pCmdBuffer->m_commandBuffer);
		}

		if (vkEndCommandBuffer(pCmdBuffer->m_commandBuffer) == VK_SUCCESS)
		{
			pCmdBuffer->m_inExecution = true;

			VkSubmitInfo submitInfo{};
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

	CommandPool_VK* CommandManager_VK::RequestExternalCommandPool()
	{
		std::lock_guard<std::mutex> lock(m_externalCommandPoolCreationMutex);

		CommandPool_VK* pPool;
		CE_NEW(pPool, CommandPool_VK, m_pDevice, CreateCommandPool(), this);
		return pPool;
	}

	void CommandManager_VK::ReturnExternalCommandBuffer(CommandBuffer_VK* pCmdBuffer)
	{
		m_inUseCommandBuffers.Push(pCmdBuffer);
	}

	void CommandManager_VK::ReturnMultipleExternalCommandBuffer(std::vector<CommandBuffer_VK*>& cmdBuffers)
	{
		m_inUseCommandBuffers.PushMultiple(cmdBuffers);
	}

	VkCommandPool CommandManager_VK::CreateCommandPool()
	{
		VkCommandPool newPool = VK_NULL_HANDLE;

		VkCommandPoolCreateInfo poolInfo{};
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

	void CommandManager_VK::SubmitCommandBufferAsync()
	{
		CommandSubmitInfo_VK* pCommandSubmitInfo;

		while (m_isRunning)
		{
			{
				std::unique_lock<std::mutex> lock(m_commandBufferSubmissionMutex);
				m_commandBufferSubmissionCv.wait(lock, [this]() { return m_commandBufferSubmissionFlag; });
				m_commandBufferSubmissionFlag = false;
			}

			while (m_commandSubmissionQueue.TryPop(pCommandSubmitInfo))
			{
				vkQueueSubmit(m_workingQueue.queue, 1, &pCommandSubmitInfo->submitInfo, VK_NULL_HANDLE);

				if (pCommandSubmitInfo->pNotifySemaphore)
				{
					pCommandSubmitInfo->pNotifySemaphore->Signal();
				}

				{
					std::lock_guard<std::mutex> guard(m_inExecutionQueueRWMutex);

					while (!pCommandSubmitInfo->queuedCmdBuffers.empty())
					{
						CommandBuffer_VK* ptrCopy = pCommandSubmitInfo->queuedCmdBuffers.front();
						m_inExecutionCommandBuffers.Push(ptrCopy);
						pCommandSubmitInfo->queuedCmdBuffers.pop();
					}
				}

				CE_DELETE(pCommandSubmitInfo);

				{
					std::lock_guard<std::mutex> guard(m_commandBufferRecycleMutex);
					m_commandBufferRecycleFlag = true;
				}
				m_commandBufferRecycleCv.notify_one();
			}

			std::this_thread::yield();
		}
	}

	void CommandManager_VK::RecycleCommandBufferAsync()
	{
		std::unordered_map<TimelineSemaphore_VK*, std::vector<CommandBuffer_VK*>> timelineGroups;

		while (m_isRunning)
		{
			{
				std::unique_lock<std::mutex> lock(m_commandBufferRecycleMutex);
				m_commandBufferRecycleCv.wait(lock, [this]() { return m_commandBufferRecycleFlag; });
				m_commandBufferRecycleFlag = false;
			}

			std::queue<CommandBuffer_VK*> inExecutionCmdBufferQueue;

			{
				std::lock_guard<std::mutex> guard(m_inExecutionQueueRWMutex);
				m_inExecutionCommandBuffers.TryPopAll(inExecutionCmdBufferQueue);
			}
			while (!inExecutionCmdBufferQueue.empty()) // Group command buffers by timeline semaphore
			{
				CommandBuffer_VK* ptrCopy = inExecutionCmdBufferQueue.front();
				DEBUG_ASSERT_CE(ptrCopy->m_pAssociatedSubmitSemaphore);
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
							LOG_ERROR("Vulkan: Command buffer " + std::to_string((uint32_t)pCmdBuffer->m_commandBuffer) + " exection timeout. Usage flag is "
								+ std::to_string(pCmdBuffer->m_usageFlags) + ". DebugID is " + std::to_string(pCmdBuffer->m_debugID) + ".");
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
}