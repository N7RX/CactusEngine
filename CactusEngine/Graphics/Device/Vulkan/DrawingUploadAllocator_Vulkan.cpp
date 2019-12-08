#define VMA_IMPLEMENTATION
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include "DrawingCommandManager_Vulkan.h"
#include "DrawingResources_Vulkan.h"

using namespace Engine;

DrawingUploadAllocator_Vulkan::DrawingUploadAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = pDevice->physicalDevice;
	createInfo.device = pDevice->logicalDevice;
	vmaCreateAllocator(&createInfo, &m_allocator);

#if defined(ENABLE_COPY_QUEUE_VK)
	m_pUploadCommandManager = pDevice->pCopyCommandManager;
#else
	m_pUploadCommandManager = pDevice->pGraphicsCommandManager;
#endif
}

DrawingUploadAllocator_Vulkan::~DrawingUploadAllocator_Vulkan()
{
	vmaDestroyAllocator(m_allocator);
}

