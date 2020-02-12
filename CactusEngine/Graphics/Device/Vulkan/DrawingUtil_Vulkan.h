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

	inline uint32_t DetermineMipmapLevels_VK(uint32_t inputSize)
	{
		uint32_t ans = 0;
		
		while (ans < 32) // Maximum of 32 miplevels
		{
			if ((1 << ans) >= inputSize)
			{
				break;
			}
			ans++;
		}

		return ans + 1;
	}

	inline VkFormat VulkanImageFormat(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case ETextureFormat::Depth:
			return VK_FORMAT_D32_SFLOAT; // Alert: this could be incompatible with current device without checking

		default:
			return VK_FORMAT_UNDEFINED;
		}
	}

	inline VkDeviceSize VulkanFormatUnitSize(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
		case ETextureFormat::Depth:
			return 4U;

		default:
			std::cerr << "Vulkan: Unhandled texture format: " << (unsigned int)format << std::endl;
			return 4U;
		}
	}

	inline VkImageLayout VulkanImageLayout(EImageLayout layout)
	{
		switch (layout)
		{
		case EImageLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;

		case EImageLayout::General:
			return VK_IMAGE_LAYOUT_GENERAL;

		case EImageLayout::ColorAttachment:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		case EImageLayout::DepthStencilAttachment:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		case EImageLayout::DepthStencilReadOnly:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		case EImageLayout::DepthReadOnlyStencilAttachment:
			return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;

		case EImageLayout::DepthAttachmentStencilReadOnly:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

		case EImageLayout::ShaderReadOnly:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		case EImageLayout::TransferSrc:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		case EImageLayout::TransferDst:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		case EImageLayout::Preinitialized:
			return VK_IMAGE_LAYOUT_PREINITIALIZED;

		case EImageLayout::PresentSrc:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		case EImageLayout::SharedPresent:
			return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;

		default:
			std::cerr << "Vulkan: Unhandled image layout: " << (unsigned int)layout << std::endl;
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	inline VkAttachmentLoadOp VulkanLoadOp(EAttachmentOperation op)
	{
		switch (op)
		{
		case EAttachmentOperation::None:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		case EAttachmentOperation::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;

		case EAttachmentOperation::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		default:
			std::cerr << "Vulkan: Unhandled load operation: " << (unsigned int)op << std::endl;
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
	}

	inline VkAttachmentStoreOp VulkanStoreOp(EAttachmentOperation op)
	{
		switch (op)
		{
		case EAttachmentOperation::None:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;

		case EAttachmentOperation::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;

		default:
			std::cerr << "Vulkan: Unhandled store operation: " << (unsigned int)op << std::endl;
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	inline VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount)
	{
		switch (sampleCount)
		{
		case 1U:
			return VK_SAMPLE_COUNT_1_BIT;

		case 2U:
			return VK_SAMPLE_COUNT_2_BIT;

		case 4U:
			return VK_SAMPLE_COUNT_4_BIT;

		case 8U:
			return VK_SAMPLE_COUNT_8_BIT;

		case 16U:
			return VK_SAMPLE_COUNT_16_BIT;

		case 32U:
			return VK_SAMPLE_COUNT_32_BIT;

		case 64U:
			return VK_SAMPLE_COUNT_64_BIT;

		default:
			std::cerr << "Vulkan: sample count out of range: " << sampleCount << std::endl;
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}
}