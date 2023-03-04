#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan.h>
#include <vector>
#include <optional>

namespace Engine
{
	struct QueueFamilyIndices_VK
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> transferFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
		}
	};

	struct SwapchainSupportDetails_VK
	{
		SwapchainSupportDetails_VK()
			: capabilities{}
		{

		}

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