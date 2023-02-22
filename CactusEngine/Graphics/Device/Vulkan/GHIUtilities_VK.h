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

	inline VkPipelineStageFlagBits DetermineShaderPipelineStage_VK(EShaderType shaderStage)
	{
		switch (shaderStage)
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
			std::cerr << "Vulkan: Unhandled shader stage: " << (unsigned int)shaderStage << std::endl;
			return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
	}

	inline VkShaderStageFlags VulkanShaderStageFlags(uint32_t flags)
	{
		assert(flags != 0);

		VkShaderStageFlags res = 0;

		if (flags & (uint32_t)EShaderType::Vertex)
		{
			res |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (flags & (uint32_t)EShaderType::Fragment)
		{
			res |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (flags & (uint32_t)EShaderType::TessControl)
		{
			res |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		}
		if (flags & (uint32_t)EShaderType::TessEvaluation)
		{
			res |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		}
		if (flags & (uint32_t)EShaderType::Geometry)
		{
			res |= VK_SHADER_STAGE_GEOMETRY_BIT;
		}
		if (flags & (uint32_t)EShaderType::Compute)
		{
			res |= VK_SHADER_STAGE_COMPUTE_BIT;
		}

		assert(res != 0);
		return res;
	}

	inline VkPipelineStageFlags VulkanPipelineStageFlags(uint32_t flags)
	{
		assert(flags != 0);

		VkShaderStageFlags res = 0;

		if (flags & (uint32_t)EShaderType::Vertex)
		{
			res |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}
		if (flags & (uint32_t)EShaderType::Fragment)
		{
			res |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		if (flags & (uint32_t)EShaderType::TessControl)
		{
			res |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
		}
		if (flags & (uint32_t)EShaderType::TessEvaluation)
		{
			res |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
		}
		if (flags & (uint32_t)EShaderType::Geometry)
		{
			res |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
		}
		if (flags & (uint32_t)EShaderType::Compute)
		{
			res |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}

		assert(res != 0);
		return res;
	}

	inline void GetAccessAndStageFromImageLayout_VK(const VkImageLayout layout, VkAccessFlags& accessMask, VkPipelineStageFlags& pipelineStage, uint32_t appliedStages)
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

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			accessMask = VK_ACCESS_TRANSFER_READ_BIT;
			pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			return;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			accessMask = VK_ACCESS_SHADER_READ_BIT;
			pipelineStage = VulkanPipelineStageFlags(appliedStages);
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
		return (uint32_t)std::floor(std::log2(inputSize)) + 1;
	}

	inline VkFormat VulkanImageFormat(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;

		case ETextureFormat::Depth:
			return VK_FORMAT_D32_SFLOAT; // Alert: this could be incompatible with current device without checking

		case ETextureFormat::RGBA8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;

		case ETextureFormat::BGRA8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;

		case ETextureFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT;

		case ETextureFormat::RG32F:
			return VK_FORMAT_R32G32_SFLOAT;

		default:
			return VK_FORMAT_UNDEFINED;
		}
	}

	inline VkDeviceSize VulkanFormatUnitSize(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
			return 16U;

		case ETextureFormat::Depth:
			return 4U;

		case ETextureFormat::RGBA8_SRGB:
		case ETextureFormat::BGRA8_UNORM:
			return 4U;

		default:
			std::cerr << "Vulkan: Unhandled texture format: " << (unsigned int)format << std::endl;
			return 4U;
		}
	}

	inline VkDeviceSize VulkanFormatUnitSize(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16U;

		case VK_FORMAT_D32_SFLOAT:
			return 4U;

		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
			return 4U;

		default:
			std::cerr << "Vulkan: Unhandled format: " << (unsigned int)format << std::endl;
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
			std::cerr << "Vulkan: Sample count out of range: " << sampleCount << std::endl;
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	inline VkPrimitiveTopology VulkanPrimitiveTopology(EAssemblyTopology topology)
	{
		switch (topology)
		{
		case EAssemblyTopology::PointList:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		case EAssemblyTopology::LineList:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		case EAssemblyTopology::LineStrip:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

		case EAssemblyTopology::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		case EAssemblyTopology::TriangleStrip:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		case EAssemblyTopology::TriangleFan:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

		default:
			std::cerr << "Vulkan: Unhandled assembly topology: " << (unsigned int)topology << std::endl;
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		}
	}

	inline VkPolygonMode VulkanPolygonMode(EPolygonMode mode)
	{
		switch (mode)
		{
		case EPolygonMode::Fill:
			return VK_POLYGON_MODE_FILL;

		case EPolygonMode::Line:
			return VK_POLYGON_MODE_LINE;

		case EPolygonMode::Point:
			return VK_POLYGON_MODE_POINT;

		default:
			std::cerr << "Vulkan: Unhandled polygon mode: " << (unsigned int)mode << std::endl;
			return VK_POLYGON_MODE_FILL;
		}
	}

	inline VkCullModeFlags VulkanCullMode(ECullMode mode)
	{
		switch (mode)
		{
		case ECullMode::None:
			return VK_CULL_MODE_NONE;

		case ECullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;

		case ECullMode::Back:
			return VK_CULL_MODE_BACK_BIT;

		case ECullMode::FrontAndBack:
			return VK_CULL_MODE_FRONT_AND_BACK;

		default:
			std::cerr << "Vulkan: Unhandled cull mode: " << (unsigned int)mode << std::endl;
			return VK_CULL_MODE_NONE;
		}
	}

	inline VkCompareOp VulkanCompareOp(ECompareOperation op)
	{
		switch (op)
		{
		case ECompareOperation::Never:
			return VK_COMPARE_OP_NEVER;

		case ECompareOperation::Less:
			return VK_COMPARE_OP_LESS;

		case ECompareOperation::Equal:
			return VK_COMPARE_OP_EQUAL;

		case ECompareOperation::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;

		case ECompareOperation::Greater:
			return VK_COMPARE_OP_GREATER;

		case ECompareOperation::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;

		case ECompareOperation::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;

		case ECompareOperation::Always:
			return VK_COMPARE_OP_ALWAYS;

		default:
			std::cerr << "Vulkan: Unhandled compare operation: " << (unsigned int)op << std::endl;
			return VK_COMPARE_OP_ALWAYS;
		}
	}

	inline VkBlendFactor VulkanBlendFactor(EBlendFactor factor)
	{
		switch (factor)
		{
		case EBlendFactor::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;

		case EBlendFactor::OneMinusSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		case EBlendFactor::One:
			return VK_BLEND_FACTOR_ONE;

		default:
			std::cerr << "Vulkan: Unhandled blend factor: " << (unsigned int)factor << std::endl;
			return VK_BLEND_FACTOR_ONE;
		}
	}

	inline VkBlendOp VulkanBlendOp(EBlendOperation op)
	{
		switch (op)
		{
		case EBlendOperation::Add:
			return VK_BLEND_OP_ADD;

		case EBlendOperation::Subtract:
			return VK_BLEND_OP_SUBTRACT;

		case EBlendOperation::Min:
			return VK_BLEND_OP_MIN;

		case EBlendOperation::Max:
			return VK_BLEND_OP_MAX;

		case EBlendOperation::Zero:
			return VK_BLEND_OP_ZERO_EXT;

		case EBlendOperation::Src:
			return VK_BLEND_OP_SRC_EXT;

		case EBlendOperation::Dst:
			return VK_BLEND_OP_DST_EXT;

		default:
			std::cerr << "Vulkan: Unhandled blend operation: " << (unsigned int)op << std::endl;
			return VK_BLEND_OP_ADD;
		}
	}

	inline VkVertexInputRate VulkanVertexInputRate(EVertexInputRate rate)
	{
		switch (rate)
		{
		case EVertexInputRate::PerVertex:
			return VK_VERTEX_INPUT_RATE_VERTEX;

		case EVertexInputRate::PerInstance:
			return VK_VERTEX_INPUT_RATE_INSTANCE;

		default:
			std::cerr << "Vulkan: Unhandled vertex input rate: " << (unsigned int)rate << std::endl;
			return VK_VERTEX_INPUT_RATE_VERTEX;
		}
	}

	inline VkDescriptorType VulkanDescriptorType(EDescriptorType type)
	{
		switch (type)
		{
		case EDescriptorType::UniformBuffer:
		case EDescriptorType::SubUniformBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		case EDescriptorType::StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		case EDescriptorType::SampledImage:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case EDescriptorType::CombinedImageSampler:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		case EDescriptorType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;

		case EDescriptorType::StorageImage:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case EDescriptorType::UniformTexelBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

		case EDescriptorType::StorageTexelBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

		default:
			std::cerr << "Vulkan: Unhandled descriptor type: " << (unsigned int)type << std::endl;
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
	}

	inline VkFilter VulkanSamplerFilterMode(ESamplerFilterMode mode)
	{
		switch (mode)
		{
		case ESamplerFilterMode::Nearest:
			return VK_FILTER_NEAREST;

		case ESamplerFilterMode::Linear:
			return VK_FILTER_LINEAR;

		case ESamplerFilterMode::Cubic:
			return VK_FILTER_CUBIC_IMG;

		default:
			std::cerr << "Vulkan: Unhandled filter mode: " << (unsigned int)mode << std::endl;
			return VK_FILTER_LINEAR;
		}
	}

	inline VkSamplerMipmapMode VulkanSamplerMipmapMode(ESamplerMipmapMode mode)
	{
		switch (mode)
		{
		case ESamplerMipmapMode::Nearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case ESamplerMipmapMode::Linear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;

		default:
			std::cerr << "Vulkan: Unhandled sampler mipmap mode: " << (unsigned int)mode << std::endl;
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	}

	inline VkSamplerAddressMode VulkanSamplerAddressMode(ESamplerAddressMode mode)
	{
		switch (mode)
		{
		case ESamplerAddressMode::Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;

		case ESamplerAddressMode::MirroredRepeat:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

		case ESamplerAddressMode::ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		case ESamplerAddressMode::ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

		case ESamplerAddressMode::MirrorClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

		default:
			std::cerr << "Vulkan: Unhandled sampler address mode: " << (unsigned int)mode << std::endl;
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	inline VkImageUsageFlags DetermineImageUsage_VK(ETextureType type)
	{
		switch (type)
		{
		case ETextureType::SampledImage:
			return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		case ETextureType::ColorAttachment:
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		case ETextureType::DepthAttachment:
			return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		default:
			std::cerr << "Vulkan: Unhandled texture type: " << (unsigned int)type << std::endl;
			return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
	}

	inline VkBufferUsageFlags VulkanDataTransferBufferUsage(uint32_t usageFlags)
	{
		assert(usageFlags != 0);
		VkBufferUsageFlags res = 0;

		if ((usageFlags & (uint32_t)EDataTransferBufferUsage::TransferSrc) != 0)
		{
			res |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		if ((usageFlags & (uint32_t)EDataTransferBufferUsage::TransferDst) != 0)
		{
			res |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		assert(res != 0);
		return res;
	}

	inline VmaMemoryUsage VulkanMemoryUsage(EMemoryType type)
	{
		switch (type)
		{
		case EMemoryType::CPU_Only:
			return VMA_MEMORY_USAGE_CPU_ONLY;

		case EMemoryType::CPU_To_GPU:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;

		case EMemoryType::GPU_Only:
			return VMA_MEMORY_USAGE_GPU_ONLY;

		case EMemoryType::GPU_To_CPU:
			return VMA_MEMORY_USAGE_GPU_TO_CPU;

		default:
			std::cerr << "Vulkan: Unhandled memory usage type: " << (unsigned int)type << std::endl;
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		}
	}

	inline VkPipelineStageFlags VulkanSemaphoreWaitStage(ESemaphoreWaitStage stage)
	{
		switch (stage)
		{
		case ESemaphoreWaitStage::TopOfPipeline:
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		case ESemaphoreWaitStage::ColorAttachmentOutput:
			return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		case ESemaphoreWaitStage::Transfer:
			return VK_PIPELINE_STAGE_TRANSFER_BIT;

		case ESemaphoreWaitStage::BottomOfPipeline:
			return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		default:
			std::cerr << "Vulkan: Unhandled semaphore wait stage: " << (unsigned int)stage << std::endl;
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
	}
}