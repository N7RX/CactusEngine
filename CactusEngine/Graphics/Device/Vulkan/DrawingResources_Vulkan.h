#pragma once
#include "DrawingResources.h"
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingUtil_Vulkan.h"
#include <vulkan.h>
#include <memory>

namespace Engine
{
	class  DrawingDevice_Vulkan;
	struct LogicalDevice_Vulkan;

	class Texture2D_Vulkan : public Texture2D
	{
	public:
		Texture2D_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const Texture2DCreateInfo_Vulkan& createInfo);
		Texture2D_Vulkan(const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat);
		~Texture2D_Vulkan();

	private:
		VkImage			m_image = VK_NULL_HANDLE;
		VkImageView		m_imageView = VK_NULL_HANDLE;
		VkImageLayout	m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocation	m_allocation = VK_NULL_HANDLE;
		VkExtent2D		m_extent = { 0, 0 };
		VkFormat		m_format = VK_FORMAT_UNDEFINED;
		uint32_t		m_mipLevels = 0;

		std::shared_ptr<DrawingDevice_Vulkan> m_pDrawingDevice;
		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;

		friend class DrawingDevice_Vulkan;
		friend class DrawingCommandBuffer_Vulkan;
		friend class DrawingUploadAllocator_Vulkan;
	};

	class RawBuffer_Vulkan : public RawResource
	{
	public:
		RawBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& createInfo);
		virtual ~RawBuffer_Vulkan();

	protected:
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
		VkDeviceSize m_deviceSize;

		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;

		friend class DrawingCommandBuffer_Vulkan;
		friend class DrawingUploadAllocator_Vulkan;
	};
}