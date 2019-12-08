#include "DrawingDescriptorAllocator_Vulkan.h"
#include "DrawingDevice_Vulkan.h"

using namespace Engine;

DrawingDescriptorAllocator_Vulkan::DrawingDescriptorAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{

}

DrawingDescriptorAllocator_Vulkan::~DrawingDescriptorAllocator_Vulkan()
{

}