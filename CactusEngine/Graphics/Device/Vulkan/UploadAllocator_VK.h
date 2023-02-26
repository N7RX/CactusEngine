#pragma once
#include <vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace Engine
{
	struct Texture2DCreateInfo_VK
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

	struct RawBufferCreateInfo_VK
	{
		VkBufferUsageFlags	usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VmaMemoryUsage		memoryUsage = VMA_MEMORY_USAGE_UNKNOWN;
		VkDeviceSize		size = 0;

		VkDeviceSize		stride = 0;
		VkIndexType			indexFormat = VK_INDEX_TYPE_UINT32;
	};

	struct LogicalDevice_VK;
	class Texture2D_VK;
	class RawBuffer_VK;
	class CommandManager_VK;
	class UploadAllocator_VK
	{
	public:
		UploadAllocator_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, VkInstance instance);
		~UploadAllocator_VK();

		bool CreateBuffer(const RawBufferCreateInfo_VK& createInfo, RawBuffer_VK& rawBuffer);
		bool CreateTexture2D(const Texture2DCreateInfo_VK& createInfo, Texture2D_VK& texture2d);

		bool MapMemory(VmaAllocation& allocation, void** mappedData);
		void UnmapMemory(VmaAllocation& allocation);

		void FreeBuffer(VkBuffer& buffer, VmaAllocation& allocation);
		void FreeImage(VkImage& image, VmaAllocation& allocation);

	private:
		std::shared_ptr<LogicalDevice_VK> m_pDevice;
		VmaAllocator m_allocator;
	};
}