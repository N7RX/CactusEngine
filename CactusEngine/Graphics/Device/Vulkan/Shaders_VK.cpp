#include "Shaders_VK.h"
#include "GraphicsHardwareInterface_VK.h"
#include "BuiltInShaderType.h"
#include "MemoryAllocator.h"

#include <cstdarg>

namespace Engine
{
	RawShader_VK::RawShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entry)
		: m_pDevice(pDevice),
		m_shaderModule(shaderModule),
		m_shaderStage(shaderStage),
		m_entryName(entry)
	{

	}

	RawShader_VK::~RawShader_VK()
	{
		m_rawCode.clear();
	}

	VertexShader_VK::VertexShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry)
	{
		CE_NEW(m_pShaderImpl, RawShader_VK, pDevice, shaderModule, VK_SHADER_STAGE_VERTEX_BIT, entry);
		m_pShaderImpl->m_rawCode = rawCode;
	}

	RawShader_VK* VertexShader_VK::GetShaderImpl() const
	{
		return m_pShaderImpl;
	}

	FragmentShader_VK::FragmentShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry)
	{
		CE_NEW(m_pShaderImpl, RawShader_VK, pDevice, shaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, entry);
		m_pShaderImpl->m_rawCode = rawCode;
	}

	RawShader_VK* FragmentShader_VK::GetShaderImpl() const
	{
		return m_pShaderImpl;
	}

	ShaderProgram_VK::ShaderProgram_VK(GraphicsHardwareInterface_VK* pDevice, LogicalDevice_VK* pLogicalDevice, uint32_t shaderCount, const RawShader_VK* pShader...)
		: ShaderProgram(0),
		m_pLogicalDevice(pLogicalDevice),
		m_pDescriptorSetLayout(nullptr),
		m_descriptorPoolCreateInfo{},
		m_descriptorSetAccessIndex(0)
	{
		m_pDevice = pDevice;

		va_list vaShaders;
		va_start(vaShaders, shaderCount); // shaderCount is the parameter preceding the first variable parameter 
		RawShader_VK* shaderPtr = nullptr;

		m_descriptorPoolCreateInfo.maxDescSetCount = DESCRIPTOR_POOL_CAPACITY;

		m_shaderStages = 0;
		while (shaderCount > 0)
		{
			shaderPtr = va_arg(vaShaders, RawShader_VK*);

			m_shaderStages |= (uint32_t)ShaderStageBitsConvert(shaderPtr->m_shaderStage);

			ReflectResources(shaderPtr, m_descriptorPoolCreateInfo);

			VkPipelineShaderStageCreateInfo shaderStageInfo{};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = shaderPtr->m_shaderStage;
			shaderStageInfo.module = shaderPtr->m_shaderModule;
			shaderStageInfo.pName = shaderPtr->m_entryName;
			m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

			shaderCount--;
		}

		va_end(vaShaders);

		CreateDescriptorSetLayout(m_descriptorPoolCreateInfo);
		CreateNewDescriptorPool();

		AllocateDescriptorSet(1);
	}

	ShaderProgram_VK::ShaderProgram_VK(GraphicsHardwareInterface_VK* pDevice, LogicalDevice_VK* pLogicalDevice, const RawShader_VK* pVertexShader, const RawShader_VK* pFragmentShader)
		: ShaderProgram(0),
		m_pLogicalDevice(pLogicalDevice),
		m_pDescriptorSetLayout(nullptr),
		m_descriptorPoolCreateInfo{},
		m_descriptorSetAccessIndex(0)
	{
		m_pDevice = pDevice;

		m_shaderStages = 0;
		m_shaderStages |= (uint32_t)ShaderStageBitsConvert(VK_SHADER_STAGE_VERTEX_BIT);
		m_shaderStages |= (uint32_t)ShaderStageBitsConvert(VK_SHADER_STAGE_FRAGMENT_BIT);

		m_descriptorPoolCreateInfo.maxDescSetCount = DESCRIPTOR_POOL_CAPACITY;

		ReflectResources(pVertexShader, m_descriptorPoolCreateInfo);

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = pVertexShader->m_shaderStage;
		shaderStageInfo.module = pVertexShader->m_shaderModule;
		shaderStageInfo.pName = pVertexShader->m_entryName;
		m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

		ReflectResources(pFragmentShader, m_descriptorPoolCreateInfo);

		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = pFragmentShader->m_shaderStage;
		shaderStageInfo.module = pFragmentShader->m_shaderModule;
		shaderStageInfo.pName = pFragmentShader->m_entryName;
		m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

		CreateDescriptorSetLayout(m_descriptorPoolCreateInfo);
		CreateNewDescriptorPool();

		AllocateDescriptorSet(1);
	}

	ShaderProgram_VK::~ShaderProgram_VK()
	{
		for (auto& stageInfo : m_pipelineShaderStageCreateInfos)
		{
			if (stageInfo.module != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(m_pLogicalDevice->logicalDevice, stageInfo.module, nullptr);
			}
		}

		for (auto& pPool : m_descriptorPools)
		{
			m_pLogicalDevice->pDescriptorAllocator->DestroyDescriptorPool(pPool);
		}
	}

	uint32_t ShaderProgram_VK::GetParamBinding(const char* paramName) const
	{
		if (m_resourceTable.find(paramName) != m_resourceTable.end())
		{
			return m_resourceTable.at(paramName).binding;
		}
		LOG_ERROR(std::string("Vulkan: Parameter name not found: ") + paramName);
		return -1;
	}

	uint32_t ShaderProgram_VK::GetStageCount() const
	{
		return (uint32_t)m_pipelineShaderStageCreateInfos.size();
	}

	const VkPipelineShaderStageCreateInfo* ShaderProgram_VK::GetShaderStageCreateInfos() const
	{
		return m_pipelineShaderStageCreateInfos.data();
	}

	DescriptorSet_VK* ShaderProgram_VK::GetDescriptorSet()
	{
		std::lock_guard<std::mutex> lock(m_descriptorSetGetMutex);

		bool flag = true;
		for (uint32_t i = m_descriptorSetAccessIndex; ; i = (i + 1) % m_descriptorSets.size())
		{
			if (!flag && i == m_descriptorSetAccessIndex)
			{
				// No available set found, allocate new one
				AllocateDescriptorSet(1);

				m_descriptorSetAccessIndex = 0;
				m_descriptorSets[m_descriptorSets.size() - 1]->m_isInUse = true;
				return m_descriptorSets[m_descriptorSets.size() - 1];
			}
			flag = false;

			if (!m_descriptorSets[i]->m_isInUse)
			{
				m_descriptorSetAccessIndex = (i + 1) % m_descriptorSets.size();
				m_descriptorSets[i]->m_isInUse = true;
				return m_descriptorSets[i];
			}
		}

		throw std::runtime_error("Vulkan: Error getting a usable descriptor set.");
		return VK_NULL_HANDLE;
	}

	const DescriptorSetLayout_VK* ShaderProgram_VK::GetDescriptorSetLayout() const
	{
		return m_pDescriptorSetLayout;
	}

	void ShaderProgram_VK::ReflectResources(const RawShader_VK* pShader, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		size_t wordCount = pShader->m_rawCode.size() * sizeof(char) / sizeof(uint32_t);
		DEBUG_ASSERT_CE(wordCount > 0);
		std::vector<uint32_t> rawCode(wordCount);
		memcpy(rawCode.data(), pShader->m_rawCode.data(), pShader->m_rawCode.size() * sizeof(char));

		spirv_cross::Compiler spvCompiler(std::move(rawCode));
		spirv_cross::ShaderResources shaderRes = spvCompiler.get_shader_resources();

		// Query only active variables
		auto activeVars = spvCompiler.get_active_interface_variables();
		spvCompiler.set_enabled_interface_variables(std::move(activeVars));

		LoadResourceBinding(spvCompiler, shaderRes);
		LoadResourceDescriptor(spvCompiler, shaderRes, ShaderStageBitsConvert(pShader->m_shaderStage), descPoolCreateInfo);
	}

	void ShaderProgram_VK::LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes)
	{
		for (auto& buffer : shaderRes.uniform_buffers)
		{
			ResourceDescription desc{};
			desc.type = EShaderResourceType_VK::Uniform;
			desc.binding = spvCompiler.get_decoration(buffer.id, spv::DecorationBinding);
			desc.name = MatchShaderParamName(buffer.name.c_str());

			m_resourceTable.emplace(desc.name, desc);
		}

		if (!shaderRes.push_constant_buffers.empty())
		{
			LOG_ERROR("Vulkan: Push constant is not supported but is used in a shader.");
		}

		for (auto& separateImage : shaderRes.separate_images)
		{
			ResourceDescription desc{};
			desc.type = EShaderResourceType_VK::SeparateImage;
			desc.binding = spvCompiler.get_decoration(separateImage.id, spv::DecorationBinding);
			desc.name = MatchShaderParamName(spvCompiler.get_name(separateImage.id).c_str());

			m_resourceTable.emplace(desc.name, desc);
		}

		for (auto& separateSampler : shaderRes.separate_samplers)
		{
			ResourceDescription desc{};
			desc.type = EShaderResourceType_VK::SeparateSampler;
			desc.binding = spvCompiler.get_decoration(separateSampler.id, spv::DecorationBinding);
			desc.name = MatchShaderParamName(spvCompiler.get_name(separateSampler.id).c_str());

			m_resourceTable.emplace(desc.name, desc);
		}

		for (auto& sampledImage : shaderRes.sampled_images)
		{
			ResourceDescription desc{};
			desc.type = EShaderResourceType_VK::SampledImage;
			desc.binding = spvCompiler.get_decoration(sampledImage.id, spv::DecorationBinding);
			desc.name = MatchShaderParamName(spvCompiler.get_name(sampledImage.id).c_str());

			m_resourceTable.emplace(desc.name, desc);
		}

		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs
	}

	void ShaderProgram_VK::LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		// TODO: eliminate duplicate descriptor set create info

		LoadUniformBuffer(spvCompiler, shaderRes, shaderType, descPoolCreateInfo);
		LoadSeparateSampler(spvCompiler, shaderRes, shaderType, descPoolCreateInfo);
		LoadSeparateImage(spvCompiler, shaderRes, shaderType, descPoolCreateInfo);
		LoadImageSampler(spvCompiler, shaderRes, shaderType, descPoolCreateInfo);

		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs
	}

	void ShaderProgram_VK::LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		uint32_t count = 0;

		for (auto& buffer : shaderRes.uniform_buffers)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1; // Alert: not sure if this is correct for uniform blocks
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.stageFlags = ShaderTypeConvertToStageBits(shaderType);
			binding.binding = spvCompiler.get_decoration(buffer.id, spv::DecorationBinding);
			binding.pImmutableSamplers = nullptr;

			if (descPoolCreateInfo.recordedLayoutBindings.find(binding.binding) == descPoolCreateInfo.recordedLayoutBindings.end())
			{
				descPoolCreateInfo.recordedLayoutBindings.emplace(binding.binding, descPoolCreateInfo.descSetLayoutBindings.size());
				descPoolCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else // Update stage flags
			{
				descPoolCreateInfo.descSetLayoutBindings[descPoolCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descPoolCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) == descPoolCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSize.descriptorCount = descPoolCreateInfo.maxDescSetCount * count;

				descPoolCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = descPoolCreateInfo.descSetPoolSizes.size(); // Record index
				descPoolCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descPoolCreateInfo.descSetPoolSizes[descPoolCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)].descriptorCount += descPoolCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		uint32_t count = 0;

		for (auto& sampler : shaderRes.separate_samplers)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			binding.stageFlags = ShaderTypeConvertToStageBits(shaderType);
			binding.binding = spvCompiler.get_decoration(sampler.id, spv::DecorationBinding);
			binding.pImmutableSamplers = nullptr;

			if (descPoolCreateInfo.recordedLayoutBindings.find(binding.binding) == descPoolCreateInfo.recordedLayoutBindings.end())
			{
				descPoolCreateInfo.recordedLayoutBindings.emplace(binding.binding, descPoolCreateInfo.descSetLayoutBindings.size());
				descPoolCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else
			{
				descPoolCreateInfo.descSetLayoutBindings[descPoolCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descPoolCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_SAMPLER) == descPoolCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
				poolSize.descriptorCount = descPoolCreateInfo.maxDescSetCount * count;

				descPoolCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_SAMPLER] = descPoolCreateInfo.descSetPoolSizes.size(); // Record index
				descPoolCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descPoolCreateInfo.descSetPoolSizes[descPoolCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_SAMPLER)].descriptorCount += descPoolCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		uint32_t count = 0;

		for (auto& image : shaderRes.separate_images)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
			binding.stageFlags = ShaderTypeConvertToStageBits(shaderType);
			binding.binding = spvCompiler.get_decoration(image.id, spv::DecorationBinding);
			binding.pImmutableSamplers = nullptr;

			if (descPoolCreateInfo.recordedLayoutBindings.find(binding.binding) == descPoolCreateInfo.recordedLayoutBindings.end())
			{
				descPoolCreateInfo.recordedLayoutBindings.emplace(binding.binding, descPoolCreateInfo.descSetLayoutBindings.size());
				descPoolCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else
			{
				descPoolCreateInfo.descSetLayoutBindings[descPoolCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descPoolCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) == descPoolCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				poolSize.descriptorCount = descPoolCreateInfo.maxDescSetCount * count;

				descPoolCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] = descPoolCreateInfo.descSetPoolSizes.size(); // Record index
				descPoolCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descPoolCreateInfo.descSetPoolSizes[descPoolCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)].descriptorCount += descPoolCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadImageSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		uint32_t count = 0;

		for (auto& sampledImage : shaderRes.sampled_images)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.stageFlags = ShaderTypeConvertToStageBits(shaderType);
			binding.binding = spvCompiler.get_decoration(sampledImage.id, spv::DecorationBinding);
			binding.pImmutableSamplers = nullptr;

			if (descPoolCreateInfo.recordedLayoutBindings.find(binding.binding) == descPoolCreateInfo.recordedLayoutBindings.end())
			{
				descPoolCreateInfo.recordedLayoutBindings.emplace(binding.binding, descPoolCreateInfo.descSetLayoutBindings.size());
				descPoolCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else // Update stage flags
			{
				descPoolCreateInfo.descSetLayoutBindings[descPoolCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descPoolCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) == descPoolCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSize.descriptorCount = descPoolCreateInfo.maxDescSetCount * count;

				descPoolCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = descPoolCreateInfo.descSetPoolSizes.size(); // Record index
				descPoolCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descPoolCreateInfo.descSetPoolSizes[descPoolCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)].descriptorCount += descPoolCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::CreateDescriptorSetLayout(const DescriptorPoolCreateInfo& descPoolCreateInfo)
	{
		CE_NEW(m_pDescriptorSetLayout, DescriptorSetLayout_VK, m_pLogicalDevice, descPoolCreateInfo.descSetLayoutBindings);
	}

	void ShaderProgram_VK::CreateNewDescriptorPool()
	{
		auto pNewPool = m_pLogicalDevice->pDescriptorAllocator->CreateDescriptorPool(m_descriptorPoolCreateInfo.maxDescSetCount, m_descriptorPoolCreateInfo.descSetPoolSizes);
		m_descriptorPools.push_back(pNewPool);
	}

	void ShaderProgram_VK::AllocateDescriptorSet(uint32_t count)
	{
		// m_descriptorSetGetMutex is already locked in GetDescriptorSet, so we don't need to lock it here

		std::vector<VkDescriptorSetLayout> layouts(count, *m_pDescriptorSetLayout->GetDescriptorSetLayout());

		for (auto& pPool : m_descriptorPools)
		{
			if (pPool->RemainingCapacity() >= count)
			{
				pPool->AllocateDescriptorSets(layouts, m_descriptorSets);
				return;
			}
		}

		// All pools are full, create a new one

		CreateNewDescriptorPool();
		m_descriptorPools.back()->AllocateDescriptorSets(layouts, m_descriptorSets);
	}

	void ShaderProgram_VK::UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_VK>& updateInfos)
	{
		DEBUG_ASSERT_CE(!m_descriptorPools.empty());

		m_descriptorPools[0]->UpdateDescriptorSets(updateInfos); // UpdateDescriptorSets does not use pool data, thus we can use any pool
	}

	uint32_t ShaderProgram_VK::GetParamTypeSize(const spirv_cross::SPIRType& type)
	{
		switch (type.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
		case spirv_cross::SPIRType::BaseType::SByte:
		case spirv_cross::SPIRType::BaseType::UByte:
			return 1U;

		case spirv_cross::SPIRType::BaseType::Short:
		case spirv_cross::SPIRType::BaseType::UShort:
		case spirv_cross::SPIRType::BaseType::Half:
			return 2U;

		case spirv_cross::SPIRType::BaseType::Int:
		case spirv_cross::SPIRType::BaseType::UInt:
		case spirv_cross::SPIRType::BaseType::Float:
			return 4U;

		case spirv_cross::SPIRType::BaseType::Int64:
		case spirv_cross::SPIRType::BaseType::UInt64:
		case spirv_cross::SPIRType::BaseType::Double:
			return 8U;

		default:
			throw std::runtime_error("Vulkan: unhandled shader parameter type.");
			break;
		}

		return 0U;
	}

	EDataType ShaderProgram_VK::BasicTypeConvert(const spirv_cross::SPIRType& type)
	{
		switch (type.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Boolean:
			return EDataType::Boolean;

		case spirv_cross::SPIRType::BaseType::Int:
			return EDataType::Int32;

		case spirv_cross::SPIRType::BaseType::UInt:
			return EDataType::UInt32;

		case spirv_cross::SPIRType::BaseType::Float:
			return EDataType::Float32;

		case spirv_cross::SPIRType::BaseType::Double:
			return EDataType::Double;

		case spirv_cross::SPIRType::BaseType::SByte:
			return EDataType::SByte;

		case spirv_cross::SPIRType::BaseType::UByte:
			return EDataType::UByte;

		case spirv_cross::SPIRType::BaseType::Short:
			return EDataType::Short;

		case spirv_cross::SPIRType::BaseType::UShort:
			return EDataType::UShort;

		case spirv_cross::SPIRType::BaseType::Half:
			return EDataType::Half;

		case spirv_cross::SPIRType::BaseType::Int64:
			return EDataType::Int64;

		case spirv_cross::SPIRType::BaseType::UInt64:
			return EDataType::UInt64;

		default:
			throw std::runtime_error("Vulkan: unhandled shader parameter base type.");
			break;
		}

		return EDataType::Float32;
	}

	EShaderType ShaderProgram_VK::ShaderStageBitsConvert(VkShaderStageFlagBits vkShaderStageBits)
	{
		switch (vkShaderStageBits)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return EShaderType::Vertex;

		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return EShaderType::TessControl;

		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return EShaderType::TessEvaluation;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return EShaderType::Geometry;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return EShaderType::Fragment;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			return EShaderType::Compute;

		case VK_SHADER_STAGE_ALL_GRAPHICS:
		case VK_SHADER_STAGE_ALL:
		case VK_SHADER_STAGE_RAYGEN_BIT_NV:
		case VK_SHADER_STAGE_ANY_HIT_BIT_NV:
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV:
		case VK_SHADER_STAGE_MISS_BIT_NV:
		case VK_SHADER_STAGE_INTERSECTION_BIT_NV:
		case VK_SHADER_STAGE_CALLABLE_BIT_NV:
		case VK_SHADER_STAGE_TASK_BIT_NV:
		case VK_SHADER_STAGE_MESH_BIT_NV:
		default:
			throw std::runtime_error("Vulkan: unhandled shader stage bit.");
			break;
		}

		return EShaderType::Undefined;
	}

	VkShaderStageFlagBits ShaderProgram_VK::ShaderTypeConvertToStageBits(EShaderType shaderType)
	{
		switch (shaderType)
		{
		case EShaderType::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;

		case EShaderType::TessControl:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

		case EShaderType::TessEvaluation:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		case EShaderType::Geometry:
			return VK_SHADER_STAGE_GEOMETRY_BIT;

		case EShaderType::Fragment:
			return VK_SHADER_STAGE_FRAGMENT_BIT;

		case EShaderType::Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;

		default:
			throw std::runtime_error("Vulkan: unhandled shader type.");
			break;
		}

		return VK_SHADER_STAGE_ALL_GRAPHICS;
	}
}