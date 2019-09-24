#pragma once
#include "SharedTypes.h"
#include <fstream>
#include <iostream>
#include <string>

namespace Engine
{
	inline void PrintPhysicalDeviceInfo_VK(const VkPhysicalDeviceProperties& properties)
	{
		std::cout << "Device: " << properties.deviceName << std::endl;
		std::cout << "Device ID: " << properties.deviceID << std::endl;

		std::string deviceType = "";
		switch (properties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			deviceType = "Other";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			deviceType = "Integrated";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			deviceType = "Discrete";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			deviceType = "Virtual";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			deviceType = "CPU";
			break;
		}

		std::cout << "Device Type: " << deviceType << std::endl;
		std::cout << "API Version: " << properties.apiVersion << std::endl;
		std::cout << "Driver Version: " << properties.driverVersion << std::endl;

		std::cout << "Max PushConstants Size: " << properties.limits.maxPushConstantsSize << std::endl;
		// Print extra limits and sparse properties here
		// ...
	}
}