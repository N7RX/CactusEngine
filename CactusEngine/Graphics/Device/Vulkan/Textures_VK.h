#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"

namespace Engine
{
	struct LogicalDevice_VK;

	enum class EAllocatorType_VK
	{
		None = 0,
		VK,  // Native Vulkan allocation
		VMA, // Managed by Vulkan Memory Allocator (VMA) library
		COUNT
	};

	class Sampler_VK : public TextureSampler
	{
	public:
		Sampler_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const VkSamplerCreateInfo& createInfo);
		~Sampler_VK();

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VkSampler m_sampler;

		friend class GraphicsHardwareInterface_VK;
		friend class Texture2D_VK;
	};

	class Texture2D_VK : public Texture2D
	{
	public:
		Texture2D_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const Texture2DCreateInfo_VK& createInfo);
		virtual ~Texture2D_VK();

		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	protected:
		Texture2D_VK();

	protected:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;

		VkImage				m_image = VK_NULL_HANDLE;
		VkImageView			m_imageView = VK_NULL_HANDLE;
		VkImageLayout		m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocation		m_allocation = VK_NULL_HANDLE;
		VkExtent2D			m_extent = { 0, 0 };
		VkFormat			m_format = VK_FORMAT_UNDEFINED;
		uint32_t			m_mipLevels = 0;
		VkImageAspectFlags	m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		EAllocatorType_VK m_allocatorType;

		std::shared_ptr<Sampler_VK> m_pSampler;

		uint32_t m_appliedStages; // EShaderType bitmap

		friend class GraphicsHardwareInterface_VK;
		friend class CommandBuffer_VK;
		friend class UploadAllocator_VK;
	};

	class RenderTarget2D_VK : public Texture2D_VK
	{
	public:
		RenderTarget2D_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat);
		~RenderTarget2D_VK();
	};

	class FrameBuffer_VK : public FrameBuffer
	{
	public:
		FrameBuffer_VK(const std::shared_ptr<LogicalDevice_VK> pDevice);
		~FrameBuffer_VK();

		uint32_t GetFrameBufferID() const override;

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VkFramebuffer m_frameBuffer;
		std::vector<std::shared_ptr<Texture2D_VK>> m_bufferAttachments;

		friend class GraphicsHardwareInterface_VK;
	};
}