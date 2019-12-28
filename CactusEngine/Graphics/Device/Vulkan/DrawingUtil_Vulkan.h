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

	inline bool HasStencilComponent_VK(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	inline VkPipelineStageFlags DetermineShaderPipelineStage_VK(EShaderType shaderType)
	{
		switch (shaderType)
		{
		case EShaderType::Vertex:
			return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

		case EShaderType::TessControl:
			return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;

		case EShaderType::TessEvaluation:
			return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

		case EShaderType::Geometry:
			return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

		case EShaderType::Fragment:
			return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		case EShaderType::Compute:
			return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		default:
			std::cerr << "Vulkan: Unhandled shader type: " << (unsigned int)shaderType << std::endl;
			return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
	}

	inline void GetAccessAndStageFromImageLayout_VK(const VkImageLayout layout, VkAccessFlags& accessMask, VkPipelineStageFlags& pipelineStage, EShaderType shaderType)
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			accessMask = 0;
			pipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			return;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			return;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			accessMask = VK_ACCESS_SHADER_READ_BIT;
			pipelineStage = DetermineShaderPipelineStage_VK(shaderType);
			return;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			accessMask = 0;
			pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			return;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			pipelineStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			return;
			// The reading happens in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage
			// and the writing in the VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT.

		default:
			std::cerr << "Vulkan: Unhandled image layout type: " << (unsigned int)layout << std::endl;
			return;
		}
	}
}