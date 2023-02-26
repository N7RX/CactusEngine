#define VMA_IMPLEMENTATION
#include "UploadAllocator_VK.h"

#include "GraphicsHardwareInterface_VK.h"
#include "CommandManager_VK.h"
#include "Buffers_VK.h"
#include "Textures_VK.h"

namespace Engine
{
	UploadAllocator_VK::UploadAllocator_VK(const std::shared_ptr<LogicalDevice_VK> pDevice, VkInstance instance)
		: m_pDevice(pDevice)
	{
		VmaAllocatorCreateInfo createInfo = {};
		createInfo.physicalDevice = pDevice->physicalDevice;
		createInfo.device = pDevice->logicalDevice;
		createInfo.instance = instance;

		if (vmaCreateAllocator(&createInfo, &m_allocator) != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan: Failed to create VMA allocator.");
		}
	}

	UploadAllocator_VK::~UploadAllocator_VK()
	{
		vmaDestroyAllocator(m_allocator);
	}

	bool UploadAllocator_VK::CreateBuffer(const RawBufferCreateInfo_VK& createInfo, RawBuffer_VK& rawBuffer)
	{
		DEBUG_ASSERT_CE(m_allocator != VK_NULL_HANDLE);

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = createInfo.size;
		bufferCreateInfo.usage = createInfo.usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = createInfo.memoryUsage;

		if (vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationInfo, &rawBuffer.m_buffer, &rawBuffer.m_allocation, nullptr) == VK_SUCCESS)
		{
			return true;
		}

		throw std::runtime_error("Vulkan: Failed to create buffer.");
		return false;
	}

	bool UploadAllocator_VK::CreateTexture2D(const Texture2DCreateInfo_VK& createInfo, Texture2D_VK& texture2d)
	{
		DEBUG_ASSERT_CE(m_allocator != VK_NULL_HANDLE);

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent = { createInfo.extent.width, createInfo.extent.height, 1 };
		imageCreateInfo.mipLevels = createInfo.mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = createInfo.format;
		imageCreateInfo.tiling = createInfo.tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = createInfo.usage;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = createInfo.memoryUsage;

		texture2d.m_width = createInfo.extent.width;
		texture2d.m_height = createInfo.extent.height;
		texture2d.m_extent = createInfo.extent;
		texture2d.m_format = createInfo.format;
		texture2d.m_mipLevels = createInfo.mipLevels;

		if (vmaCreateImage(m_allocator, &imageCreateInfo, &allocationInfo, &texture2d.m_image, &texture2d.m_allocation, nullptr) == VK_SUCCESS)
		{
			return true;
		}

		throw std::runtime_error("Vulkan: Failed to create texture 2D.");
		return false;
	}

	bool UploadAllocator_VK::MapMemory(VmaAllocation& allocation, void** mappedData)
	{
		if (vmaMapMemory(m_allocator, allocation, mappedData) != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan: Failed to map CPU memory.");
			return false;
		}
		return true;
	}

	void UploadAllocator_VK::UnmapMemory(VmaAllocation& allocation)
	{
		vmaUnmapMemory(m_allocator, allocation);
	}

	void UploadAllocator_VK::FreeBuffer(VkBuffer& buffer, VmaAllocation& allocation)
	{
		DEBUG_ASSERT_CE(m_allocator != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE);
		vmaDestroyBuffer(m_allocator, buffer, allocation);
	}

	void UploadAllocator_VK::FreeImage(VkImage& image, VmaAllocation& allocation)
	{
		DEBUG_ASSERT_CE(m_allocator != VK_NULL_HANDLE && image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE);
		vmaDestroyImage(m_allocator, image, allocation);
	}
}