#pragma once
#include <vulkan.h>
#include <vector>
#include <memory>

namespace Engine
{
	struct LogicalDevice_Vulkan;
	class DrawingDescriptorAllocator_Vulkan
	{
	public:
		DrawingDescriptorAllocator_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~DrawingDescriptorAllocator_Vulkan();

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
	};
}