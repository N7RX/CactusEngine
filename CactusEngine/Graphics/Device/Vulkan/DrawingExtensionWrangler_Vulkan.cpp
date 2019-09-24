#include "DrawingExtensionWrangler_Vulkan.h"
#include <iostream>
#include <set>

using namespace Engine;

bool Engine::CheckValidationLayerSupport_VK(const std::vector<const char*>& validationLayers)
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
	std::cerr << "Vulkan validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT Engine::GetDebugUtilsMessengerCreateInfo_VK()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;

	return createInfo;
}

VkResult Engine::CreateDebugUtilsMessengerEXT_VK(const VkInstance& instance, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, &GetDebugUtilsMessengerCreateInfo_VK(), pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Engine::DestroyDebugUtilMessengerEXT_VK(const VkInstance& instance, VkDebugUtilsMessengerEXT* pDebugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, *pDebugMessenger, pAllocator);
	}
}

bool Engine::CheckAvailableInstanceExtensions_VK(std::vector<VkExtensionProperties>& availableExtensions)
{
	uint32_t extensionsCount = 0;
	VkResult result = VK_SUCCESS;

	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

	if ((result != VK_SUCCESS) || extensionsCount == 0)
	{
		throw std::runtime_error("Vulkan: Could not get the number of instance extensions.");
		return false;
	}

	availableExtensions.resize(extensionsCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: Could not enumerate instance extensions.");
		return false;
	}

	return true;
}

bool Engine::IsExtensionSupported_VK(const std::vector<VkExtensionProperties>& availableExtensions, const char desiredExtension[VK_MAX_EXTENSION_NAME_SIZE])
{
	for (auto& extension : availableExtensions)
	{
		if (strcmp(desiredExtension, extension.extensionName) == 0)
		{
			return true;
		}
	}
	return false;
}

bool Engine::CheckDeviceExtensionsSupport_VK(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount = 0;

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

std::vector<const char*> Engine::GetRequiredExtensions_VK(bool enableValidationLayers)
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

	return extensions;
}

bool Engine::IsPhysicalDeviceSuitable_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface, const std::vector<const char*>& deviceExtensions)
{
	QueueFamilyIndices_VK indices = FindQueueFamilies_VK(device, surface);

	bool extensionsSupported = CheckDeviceExtensionsSupport_VK(device, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails_VK swapChainSupport = QuerySwapchainSupport_VK(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// Add extra selection criteria here
	// ...

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices_VK Engine::FindQueueFamilies_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	QueueFamilyIndices_VK indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t index = 0; index < static_cast<uint32_t>(queueFamilies.size()); ++index)
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

#if defined(ENABLE_COPY_QUEUE_VK)
		if (queueFamilies[index].queueCount > 0 && (queueFamilies[index].queueFlags & VK_QUEUE_TRANSFER_BIT)
			&& !(queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			indices.copyFamily = index;
		}
#endif
		if (indices.isComplete())
		{
			break;
		}
	}

	if (!indices.isComplete())
	{
		throw std::runtime_error("Vulkan: Could not find required queues.");
	}

	return indices;
}

SwapchainSupportDetails_VK Engine::QuerySwapchainSupport_VK(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
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