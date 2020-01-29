#pragma once
#include <vulkan.h>
#include <memory>
#include <vector>
#include <vk_mem_alloc.h>

namespace Engine
{
	struct Texture2DCreateInfo_Vulkan
	{
		VkExtent2D			extent = { 0, 0 };
		VkFormat			format = VK_FORMAT_UNDEFINED;
		VkImageTiling		tiling = VK_IMAGE_TILING_OPTIMAL;
		uint32_t			mipLevels = 1;
		VkImageAspectFlags	aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType		viewType = VK_IMAGE_VIEW_TYPE_2D;
		VkImageUsageFlags	usage = 0;
		VmaMemoryUsage		memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
	};

	struct RawBufferCreateInfo_Vulkan
	{
		VkBufferUsageFlags	usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VmaMemoryUsage		memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
		VkDeviceSize		size = 0;

		VkDeviceSize		stride = 0;
		VkIndexType			indexFormat = VK_INDEX_TYPE_UINT32;
	};

	struct LogicalDevice_Vulkan;
	class Texture2D_Vulkan;	
	class RawBuffer_Vulkan;	
	class DrawingCommandManager_Vulkan;
	class DrawingUploadAllocator_Vulkan
	{
	public:
		DrawingUploadAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingUploadAllocator_Vulkan();

		bool CreateBuffer(const RawBufferCreateInfo_Vulkan& createInfo, RawBuffer_Vulkan& rawBuffer);
		bool CreateTexture2D(const Texture2DCreateInfo_Vulkan& createInfo, Texture2D_Vulkan& texture2d);

		bool MapMemory(VmaAllocation& allocation, void** mappedData);
		void UnmapMemory(VmaAllocation& allocation);

		void FreeBuffer(VkBuffer& buffer, VmaAllocation& allocation);
		void FreeImage(VkImage& image, VmaAllocation& allocation);

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<DrawingCommandManager_Vulkan> m_pUploadCommandManager;
		VmaAllocator m_allocator;
	};
}