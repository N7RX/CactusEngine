#pragma once
#include <vulkan.h>
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

namespace Engine
{
	struct RawImageCreateInfo_Vulkan
	{
		VkExtent3D			extent = { 0, 0, 1 };
		VkFormat			format = VK_FORMAT_UNDEFINED;
		VkImageTiling		tiling = VK_IMAGE_TILING_OPTIMAL;
		uint32_t			mipLevels = 1;
		VkImageAspectFlags	aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType		viewType = VK_IMAGE_VIEW_TYPE_2D;
		VkImageUsageFlags	usage = 0;
		VmaMemoryUsage		memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
	};

	class Texture2D_Vulkan;	
	class Buffer_Vulkan;
	struct LogicalDevice_Vulkan;
	class DrawingCommandManager_Vulkan;
	class DrawingUploadAllocator_Vulkan
	{
	public:
		DrawingUploadAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingUploadAllocator_Vulkan();

		bool CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
		bool CreateImage(const RawImageCreateInfo_Vulkan& createInfo, Texture2D_Vulkan& rawImage);

		bool CopyBuffer_Immediate(const VkBuffer& src, VkBuffer& dst, const VkDeviceSize size);
		bool CopyBufferToImage_Immediate(const VkBuffer& buffer, VkImage& image, const std::vector<VkBufferImageCopy>& regions);

		bool CopyBuffer_Recorded(const std::shared_ptr<Buffer_Vulkan> src, VkBuffer& dst, const VkDeviceSize size);
		bool CopyBufferToImage_Recorded(const std::shared_ptr<Buffer_Vulkan> buffer, VkImage& image, const std::vector<VkBufferImageCopy>& regions);

		bool MapMemory(VmaAllocation& allocation, void** mappedData);
		bool UnmapMemory(VmaAllocation& allocation);

		bool FreeBuffer(VkBuffer& buffer, VmaAllocation& allocation);
		bool FreeImage(VkImage& image, VmaAllocation& allocation);

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<DrawingCommandManager_Vulkan> m_pUploadCommandManager;
		VmaAllocator m_allocator;
	};
}