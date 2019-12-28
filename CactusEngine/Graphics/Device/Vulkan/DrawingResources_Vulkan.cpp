#include "DrawingResources_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include "BuiltInShaderType.h"

using namespace Engine;

Texture2D_Vulkan::Texture2D_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const Texture2DCreateInfo_Vulkan& createInfo)
	: m_pDrawingDevice(pDrawingDevice), m_pLogicalDevice(pLogicalDevice)
{
	assert(pLogicalDevice);
	assert(pDrawingDevice);

	pLogicalDevice->pUploadAllocator->CreateTexture2D(createInfo, *this);

	m_format = createInfo.format;
	m_extent = createInfo.extent;
	m_mipLevels = createInfo.mipLevels;

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_image;
	viewCreateInfo.viewType = createInfo.viewType;
	viewCreateInfo.format = createInfo.format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.subresourceRange.aspectMask = createInfo.aspect;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	m_pDrawingDevice->CreateImageView(pLogicalDevice, viewCreateInfo, m_imageView);
}

Texture2D_Vulkan::Texture2D_Vulkan(const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat)
	: m_image(targetImage), m_imageView(targetView), m_extent(targetExtent), m_format(targetFormat)
{

}

Texture2D_Vulkan::~Texture2D_Vulkan()
{
	// Alert: we are missing some necessary info here
	if (m_pLogicalDevice)
	{
		m_pLogicalDevice->pUploadAllocator->FreeImage(m_image, m_allocation);
	}
	else
	{
		// Destroy image from swapchain
	}

	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pLogicalDevice->logicalDevice, m_imageView, nullptr);
	}
}

RawBuffer_Vulkan::RawBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& createInfo)
	: m_pDevice(pDevice)
{
	assert(pDevice);

	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;

	m_pDevice->pUploadAllocator->CreateBuffer(createInfo, *this);
}

RawBuffer_Vulkan::~RawBuffer_Vulkan()
{
	m_pDevice->pUploadAllocator->FreeBuffer(m_buffer, m_allocation);
}