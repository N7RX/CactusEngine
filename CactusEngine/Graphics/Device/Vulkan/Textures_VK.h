#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"

namespace Engine
{
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
		Sampler_VK(LogicalDevice_VK* pDevice, const VkSamplerCreateInfo& createInfo);
		~Sampler_VK();

	private:
		LogicalDevice_VK* m_pDevice;
		VkSampler m_sampler;

		friend class GraphicsHardwareInterface_VK;
		friend class Texture2D_VK;
	};

	class Texture2D_VK : public Texture2D
	{
	public:
		Texture2D_VK(LogicalDevice_VK* pDevice, const Texture2DCreateInfo_VK& createInfo);
		virtual ~Texture2D_VK();

		bool HasSampler() const override;
		void SetSampler(const TextureSampler* pSampler) override;
		TextureSampler* GetSampler() const override;

	protected:
		Texture2D_VK();

	protected:
		LogicalDevice_VK* m_pDevice;

		VkImage				m_image;
		VkImageView			m_imageView;
		VkImageLayout		m_layout;
		VmaAllocation		m_allocation;
		VkExtent2D			m_extent;
		VkFormat			m_format;
		uint32_t			m_mipLevels;
		VkImageAspectFlags	m_aspect;

		EAllocatorType_VK m_allocatorType;

		Sampler_VK* m_pSampler;

		uint32_t m_appliedStages; // EShaderType bitmap

		friend class GraphicsHardwareInterface_VK;
		friend class CommandBuffer_VK;
		friend class UploadAllocator_VK;
	};

	class RenderTarget2D_VK : public Texture2D_VK
	{
	public:
		RenderTarget2D_VK(LogicalDevice_VK* pDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat, bool isSwapchainImage = false);
		~RenderTarget2D_VK();

	private:
		bool m_isSwapchainImage;
	};

	class FrameBuffer_VK : public FrameBuffer
	{
	public:
		FrameBuffer_VK(LogicalDevice_VK* pDevice);
		~FrameBuffer_VK();

		uint32_t GetFrameBufferID() const override;

	private:
		LogicalDevice_VK* m_pDevice;
		VkFramebuffer m_frameBuffer;
		std::vector<Texture2D_VK*> m_bufferAttachments;

		friend class GraphicsHardwareInterface_VK;
	};
}