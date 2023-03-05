#define VOLK_IMPLEMENTATION
#include "ExtensionLoader_VK.h"
#include "LogUtility.h"

#include <set>

namespace Engine
{
	bool CheckValidationLayerSupport_VK(const std::vector<const char*>& validationLayers)
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool layerFound = false;
		for (const char* layerName : validationLayers)
		{
			layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		// TODO: filter validation layer message by type
		LOG_MESSAGE((std::string)"Vulkan validation layer: " + pCallbackData->pMessage);
		return VK_FALSE;
	}

	VkDebugUtilsMessengerCreateInfoEXT GetDebugUtilsMessengerCreateInfo_VK()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;

		return createInfo;
	}

	VkResult CreateDebugUtilsMessengerEXT_VK(const VkInstance& instance, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			auto debugUtilsMessengerCreateInfo = GetDebugUtilsMessengerCreateInfo_VK();
			return func(instance, &debugUtilsMessengerCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilMessengerEXT_VK(const VkInstance& instance, VkDebugUtilsMessengerEXT* pDebugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, *pDebugMessenger, pAllocator);
		}
	}

	bool CheckAvailableInstanceExtensions_VK(std::vector<VkExtensionProperties>& availableExtensions)
	{
		uint32_t extensionsCount = 0;
		VkResult result = VK_SUCCESS;

		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

		if ((result != VK_SUCCESS) || extensionsCount == 0)
		{
			LOG_ERROR("Vulkan: Could not get the number of instance extensions.");
			return false;
		}

		availableExtensions.resize(extensionsCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan: Could not enumerate instance extensions.");
			return false;
		}

		return true;
	}

	bool IsExtensionSupported_VK(const std::vector<VkExtensionProperties>& availableExtensions, const char desiredExtension[VK_MAX_EXTENSION_NAME_SIZE])
	{
		for (auto& extension : availableExtensions)
		{
			if (strcmp(desiredExtension, extension.extensionName) == 0)
			{
				DEBUG_LOG_MESSAGE((std::string)"Vulkan: requested extension: " + desiredExtension);
				return true;
			}
		}
		return false;
	}

	bool CheckDeviceExtensionsSupport_VK(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions)
	{
		uint32_t extensionCount = 0;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
#if defined(DEBUG_MODE_CE)
			uint32_t prevCount = requiredExtensions.size();
			requiredExtensions.erase(extension.extensionName);
			if (prevCount != requiredExtensions.size())
			{
				DEBUG_LOG_MESSAGE((std::string)"Vulkan: requested extension: " + extension.extensionName);
			}
#else
			requiredExtensions.erase(extension.extensionName);
#endif
		}

		return requiredExtensions.empty();
	}

	std::vector<const char*> GetRequiredInstanceExtensions_VK(bool enableValidationLayers)
	{
		std::vector<const char*> extensions;

		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
		extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
		extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);

#elif defined(VK_USE_PLATFORM_XCB_KHR)
		extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
		extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

#elif defined(VK_USE_PLATFORM_IOS_MVK)
		extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);

#elif defined(VK_USE_PLATFORM_MACOS_MVK)
		extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		// For VMA
#if defined(VK_KHR_get_physical_device_properties2)
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

		return extensions;
	}

	std::vector<const char*> GetRequiredDeviceExtensions_VK()
	{
		std::vector<const char*> extensions;

		extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		extensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

		// For VMA
#if defined(VK_KHR_get_memory_requirements2) && defined(VK_KHR_dedicated_allocation)
		extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
#endif
#if defined(VK_KHR_bind_memory2)
		extensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
#endif
#if defined(VK_EXT_memory_budget)
		extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
#endif
#if defined(VK_KHR_maintenance4)
		extensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
#endif

		return extensions;
	}

	std::vector<const char*> GetValidationLayerNames_VK()
	{
		std::vector<const char*> layers;

		layers.push_back("VK_LAYER_KHRONOS_validation");

		return layers;
	}

	bool IsPhysicalDeviceSuitable_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, const std::vector<const char*>& deviceExtensions)
	{
		if (device == VK_NULL_HANDLE || surface == VK_NULL_HANDLE)
		{
			return false;
		}

		//QueueFamilyIndices_VK indices = FindQueueFamilies_VK(device, surface);

		bool extensionsSupported = CheckDeviceExtensionsSupport_VK(device, deviceExtensions);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapchainSupportDetails_VK swapChainSupport = QuerySwapchainSupport_VK(device, surface);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// Add extra selection criteria here
		// ...

		//return indices.isComplete() && extensionsSupported && swapChainAdequate;
		return extensionsSupported && swapChainAdequate;
	}

	QueueFamilyIndices_VK FindQueueFamilies_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		QueueFamilyIndices_VK indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t index = 0; index < (uint32_t)queueFamilies.size(); ++index)
		{
			if (queueFamilies[index].queueCount > 0 && queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = index;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);

			if (queueFamilies[index].queueCount > 0 && presentSupport && !indices.presentFamily.has_value())
			{
				indices.presentFamily = index;
			}

			if (queueFamilies[index].queueCount > 0 && (queueFamilies[index].queueFlags & VK_QUEUE_TRANSFER_BIT)
				&& !(queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				indices.transferFamily = index;
			}

			if (indices.isComplete())
			{
				break;
			}
		}

		if (!indices.isComplete())
		{
			LOG_ERROR("Vulkan: Some queue family is not supported by a physical device.");
		}

		return indices;
	}

	SwapchainSupportDetails_VK QuerySwapchainSupport_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		SwapchainSupportDetails_VK details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount > 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount > 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
}