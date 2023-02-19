#include "Swapchain_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "Textures_VK.h"
#include "GHIUtilities_VK.h"

using namespace Engine;

Swapchain_VK::Swapchain_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const SwapchainCreateInfo_VK& createInfo)
	: m_pDevice(pDevice), m_presentQueue(VK_NULL_HANDLE), m_targetImageIndex(0), m_swapchain(VK_NULL_HANDLE)
{
	uint32_t imageCount = createInfo.supportDetails.capabilities.minImageCount - 1 + createInfo.maxFramesInFlight;
	if (createInfo.supportDetails.capabilities.maxImageCount > 0 && imageCount > createInfo.supportDetails.capabilities.maxImageCount)
	{
		imageCount = createInfo.supportDetails.capabilities.maxImageCount;
	}

	m_swapExtent = createInfo.swapExtent;

	VkSwapchainCreateInfoKHR createInfoKHR = {};
	createInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfoKHR.surface = createInfo.surface;
	createInfoKHR.minImageCount = imageCount;
	createInfoKHR.imageFormat = createInfo.surfaceFormat.format;
	createInfoKHR.imageColorSpace = createInfo.surfaceFormat.colorSpace;
	createInfoKHR.imageExtent = createInfo.swapExtent;
	createInfoKHR.imageArrayLayers = 1;
	createInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfoKHR.preTransform = createInfo.supportDetails.capabilities.currentTransform;
	createInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfoKHR.presentMode = createInfo.presentMode;
	createInfoKHR.clipped = VK_TRUE;
	createInfoKHR.oldSwapchain = VK_NULL_HANDLE;

	uint32_t queueFamilyIndices[] = { createInfo.queueFamilyIndices.graphicsFamily.value(), createInfo.queueFamilyIndices.presentFamily.value() };

	if (queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		createInfoKHR.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfoKHR.queueFamilyIndexCount = 2;
		createInfoKHR.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(m_pDevice->logicalDevice, &createInfoKHR, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: Failed to create swapchain.\n";
		return;
	}

	vkGetDeviceQueue(m_pDevice->logicalDevice, queueFamilyIndices[1], 0, &m_presentQueue);

	std::vector<VkImage> swapchainImages(imageCount, VK_NULL_HANDLE);
	if (vkGetSwapchainImagesKHR(m_pDevice->logicalDevice, m_swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: Failed to retrieve swapchain images.\n";
		return;
	}

	for (unsigned int i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = createInfo.surfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView swapchainImageView = VK_NULL_HANDLE;
		if (vkCreateImageView(m_pDevice->logicalDevice, &imageViewCreateInfo, nullptr, &swapchainImageView) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: Failed to create image view for swapchain image.\n";
			return;
		}

		m_renderTargets.emplace_back(std::make_shared<RenderTarget2D_VK>(m_pDevice, swapchainImages[i], swapchainImageView, createInfo.swapExtent, createInfo.surfaceFormat.format));
	}

	m_imageAvailableSemaphores.resize(GraphicsHardwareInterface_VK::MAX_FRAME_IN_FLIGHT, nullptr);
	for (unsigned int i = 0; i < GraphicsHardwareInterface_VK::MAX_FRAME_IN_FLIGHT; i++)
	{
		m_imageAvailableSemaphores[i] = m_pDevice->pSyncObjectManager->RequestSemaphore();
		m_imageAvailableSemaphores[i]->waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	auto pCmdBuffer = m_pDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
	for (unsigned int i = 0; i < imageCount; i++)
	{
		pCmdBuffer->TransitionImageLayout(m_renderTargets[i], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0);
	}

	m_pDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);
}

Swapchain_VK::~Swapchain_VK()
{
	m_renderTargets.clear();

	vkDestroySwapchainKHR(m_pDevice->logicalDevice, m_swapchain, nullptr);
}

bool Swapchain_VK::UpdateBackBuffer(unsigned int currentFrame)
{
	assert(currentFrame < GraphicsHardwareInterface_VK::MAX_FRAME_IN_FLIGHT);
	return vkAcquireNextImageKHR(m_pDevice->logicalDevice, m_swapchain, ACQUIRE_IMAGE_TIMEOUT, m_imageAvailableSemaphores[currentFrame]->semaphore, VK_NULL_HANDLE, &m_targetImageIndex) == VK_SUCCESS;
}

bool Swapchain_VK::Present(const std::vector<std::shared_ptr<Semaphore_VK>>& waitSemaphores)
{
	assert(m_swapchain != VK_NULL_HANDLE);
	assert(m_presentQueue != VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	std::vector<VkSemaphore> semaphoresToWait;
	for (auto& pSemaphore : waitSemaphores)
	{
		semaphoresToWait.emplace_back(pSemaphore->semaphore);
	}
	presentInfo.waitSemaphoreCount = (uint32_t)semaphoresToWait.size();
	presentInfo.pWaitSemaphores = semaphoresToWait.data();

	// TODO: we can present multiple swapchains at once if needed
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_targetImageIndex;

	return vkQueuePresentKHR(m_presentQueue, &presentInfo) == VK_SUCCESS;
}

uint32_t Swapchain_VK::GetSwapchainImageCount() const
{
	return (uint32_t)m_renderTargets.size();
}

std::shared_ptr<RenderTarget2D_VK> Swapchain_VK::GetTargetImage() const
{
	assert(m_targetImageIndex < m_renderTargets.size());
	return m_renderTargets[m_targetImageIndex];
}

uint32_t Swapchain_VK::GetTargetImageIndex() const
{
	return m_targetImageIndex;
}

std::shared_ptr<RenderTarget2D_VK> Swapchain_VK::GetSwapchainImageByIndex(unsigned int index) const
{
	assert(index < m_renderTargets.size());
	return m_renderTargets[index];
}

VkExtent2D Swapchain_VK::GetSwapExtent() const
{
	return m_swapExtent;
}

std::shared_ptr<Semaphore_VK> Swapchain_VK::GetImageAvailableSemaphore(unsigned int currentFrame) const
{
	assert(currentFrame < GraphicsHardwareInterface_VK::MAX_FRAME_IN_FLIGHT);
	return m_imageAvailableSemaphores[currentFrame];
}