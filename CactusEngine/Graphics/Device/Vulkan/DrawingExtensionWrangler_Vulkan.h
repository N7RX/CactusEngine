#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
//#define ENABLE_COPY_QUEUE_VK // From my previous experience at Autodesk, enabling copy queue would bring down performance unless you have huge amount of transfer tasks
							   // Therefore, it will be disabled for now
#define ENABLE_HETEROGENEOUS_GPUS_VK
#define ENABLE_SHADER_REFLECT_OUTPUT_VK

#include <vulkan.h>
#include <vector>
#include <optional>

namespace Engine
{
	struct QueueFamilyIndices_VK
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
#if defined(ENABLE_COPY_QUEUE_VK)
		std::optional<uint32_t> copyFamily;
#endif

		bool isComplete()
		{
#if defined(ENABLE_COPY_QUEUE_VK)
			return graphicsFamily.has_value() && presentFamily.has_value() && copyFamily.has_value();
#else
			return graphicsFamily.has_value() && presentFamily.has_value();
#endif
		}
	};

	struct SwapchainSupportDetails_VK
	{
		VkSurfaceCapabilitiesKHR		capabilities;
		std::vector<VkSurfaceFormatKHR>	formats;
		std::vector<VkPresentModeKHR>	presentModes;
	};

	// Debug messenger
	bool CheckValidationLayerSupport_VK(const std::vector<const char*>& validationLayers);
	VkDebugUtilsMessengerCreateInfoEXT GetDebugUtilsMessengerCreateInfo_VK();
	VkResult CreateDebugUtilsMessengerEXT_VK(const VkInstance& instance, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilMessengerEXT_VK(const VkInstance& instance, VkDebugUtilsMessengerEXT* pDebugMessenger, const VkAllocationCallbacks* pAllocator);

	// Extensions
	bool CheckAvailableInstanceExtensions_VK(std::vector<VkExtensionProperties>& availableExtensions);
	bool IsExtensionSupported_VK(const std::vector<VkExtensionProperties>& availableExtensions, const char desiredExtension[VK_MAX_EXTENSION_NAME_SIZE]);
	bool CheckDeviceExtensionsSupport_VK(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions);
	std::vector<const char*> GetRequiredExtensions_VK(bool enableValidationLayers = false);

	// Device
	bool IsPhysicalDeviceSuitable_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, const std::vector<const char*>& deviceExtensions);
	QueueFamilyIndices_VK FindQueueFamilies_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
	SwapchainSupportDetails_VK QuerySwapchainSupport_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
}