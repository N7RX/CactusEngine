#include "Textures_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GHIUtilities_VK.h"

namespace Engine
{
	Sampler_VK::Sampler_VK(LogicalDevice_VK* pDevice, const VkSamplerCreateInfo& createInfo)
		: m_pDevice(pDevice)
	{
		m_sampler = VK_NULL_HANDLE;

		if (vkCreateSampler(m_pDevice->logicalDevice, &createInfo, nullptr, &m_sampler) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan: Failed to create image sampler.");
		}
	}

	Sampler_VK::~Sampler_VK()
	{
		// TODO: add explicit sampler destroy function in GHI instead
		DEBUG_ASSERT_CE(m_sampler != VK_NULL_HANDLE);
		//vkDestroySampler(m_pDevice->logicalDevice, m_sampler, nullptr);
		LOG_MESSAGE("Vulkan: Sampler destructor called");
	}

	Texture2D_VK::Texture2D_VK(LogicalDevice_VK* pDevice, const Texture2DCreateInfo_VK& createInfo)
		: Texture2D(ETexture2DSource::RawDeviceTexture), m_pDevice(pDevice), m_allocatorType(EAllocatorType_VK::VMA), m_appliedStages(0)
	{
		DEBUG_ASSERT_CE(pDevice);

		m_pDevice->pUploadAllocator->CreateTexture2D(createInfo, *this);

		m_format = createInfo.format;
		m_extent = createInfo.extent;
		m_mipLevels = createInfo.mipLevels;
		m_aspect = createInfo.aspect;

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

		if (vkCreateImageView(m_pDevice->logicalDevice, &viewCreateInfo, nullptr, &m_imageView) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan: failed to create image view for texture 2D.");
		}
	}

	Texture2D_VK::Texture2D_VK()
		: Texture2D(ETexture2DSource::RawDeviceTexture)
	{

	}

	Texture2D_VK::~Texture2D_VK()
	{
		if (m_allocatorType == EAllocatorType_VK::VMA)
		{
			m_pDevice->pUploadAllocator->FreeImage(m_image, m_allocation);
		}

		if (m_imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_pDevice->logicalDevice, m_imageView, nullptr);
		}
	}

	bool Texture2D_VK::HasSampler() const
	{
		return m_pSampler != nullptr;
	}

	void Texture2D_VK::SetSampler(const TextureSampler* pSampler)
	{
		m_pSampler = (Sampler_VK*)pSampler;
	}

	TextureSampler* Texture2D_VK::GetSampler() const
	{
		return m_pSampler;
	}

	RenderTarget2D_VK::RenderTarget2D_VK(LogicalDevice_VK* pDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat)
	{
		m_pDevice = pDevice;

		m_image = targetImage;
		m_imageView = targetView;
		m_extent = targetExtent;
		m_format = targetFormat;
		m_mipLevels = 1;

		m_allocatorType = EAllocatorType_VK::VK;
	}

	RenderTarget2D_VK::~RenderTarget2D_VK()
	{
		if (m_image != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_pDevice->logicalDevice, m_image, nullptr);
		}
		if (m_imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_pDevice->logicalDevice, m_imageView, nullptr);
		}
	}

	FrameBuffer_VK::FrameBuffer_VK(LogicalDevice_VK* pDevice)
		: m_pDevice(pDevice)
	{

	}

	FrameBuffer_VK::~FrameBuffer_VK()
	{
		vkDestroyFramebuffer(m_pDevice->logicalDevice, m_frameBuffer, nullptr);
	}

	uint32_t FrameBuffer_VK::GetFrameBufferID() const
	{
		return -1; // Vulkan device does not make use of this ID
	}
}