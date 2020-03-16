#define VMA_IMPLEMENTATION
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include "DrawingCommandManager_Vulkan.h"
#include "DrawingResources_Vulkan.h"
#include <assert.h>

using namespace Engine;

DrawingUploadAllocator_Vulkan::DrawingUploadAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = pDevice->physicalDevice;
	createInfo.device = pDevice->logicalDevice;

	if (vmaCreateAllocator(&createInfo, &m_allocator) != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: Failed to create VMA allocator.");
	}
}

DrawingUploadAllocator_Vulkan::~DrawingUploadAllocator_Vulkan()
{
	vmaDestroyAllocator(m_allocator);
}

bool DrawingUploadAllocator_Vulkan::CreateBuffer(const RawBufferCreateInfo_Vulkan& createInfo, RawBuffer_Vulkan& rawBuffer)
{
	assert(m_allocator != VK_NULL_HANDLE);

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

bool DrawingUploadAllocator_Vulkan::CreateTexture2D(const Texture2DCreateInfo_Vulkan& createInfo, Texture2D_Vulkan& texture2d)
{
	assert(m_allocator != VK_NULL_HANDLE);

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

bool DrawingUploadAllocator_Vulkan::MapMemory(VmaAllocation& allocation, void** mappedData)
{
	if (vmaMapMemory(m_allocator, allocation, mappedData) != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: Failed to map CPU memory.");
		return false;
	}
	return true;
}

void DrawingUploadAllocator_Vulkan::UnmapMemory(VmaAllocation& allocation)
{
	vmaUnmapMemory(m_allocator, allocation);
}

void DrawingUploadAllocator_Vulkan::FreeBuffer(VkBuffer& buffer, VmaAllocation& allocation)
{
	assert(m_allocator != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE);
	vmaDestroyBuffer(m_allocator, buffer, allocation);
}

void DrawingUploadAllocator_Vulkan::FreeImage(VkImage& image, VmaAllocation& allocation)
{
	assert(m_allocator != VK_NULL_HANDLE && image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE);
	vmaDestroyImage(m_allocator, image, allocation);
}