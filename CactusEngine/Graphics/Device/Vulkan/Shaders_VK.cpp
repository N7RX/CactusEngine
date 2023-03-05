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
		: ShaderProgram(0), m_pLogicalDevice(pLogicalDevice), m_descriptorSetAccessIndex(0)
	{
		m_pDevice = pDevice;

		va_list vaShaders;
		va_start(vaShaders, shaderCount); // shaderCount is the parameter preceding the first variable parameter 
		RawShader_VK* shaderPtr = nullptr;

		DescriptorSetCreateInfo descSetCreateInfo{};
		descSetCreateInfo.maxDescSetCount = MAX_DESCRIPTOR_SET_COUNT;

		m_shaderStages = 0;
		while (shaderCount > 0)
		{
			shaderPtr = va_arg(vaShaders, RawShader_VK*);

			m_shaderStages |= (uint32_t)ShaderStageBitsConvert(shaderPtr->m_shaderStage);

			ReflectResources(shaderPtr, descSetCreateInfo);

			VkPipelineShaderStageCreateInfo shaderStageInfo{};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = shaderPtr->m_shaderStage;
			shaderStageInfo.module = shaderPtr->m_shaderModule;
			shaderStageInfo.pName = shaderPtr->m_entryName;
			m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

			shaderCount--;
		}

		va_end(vaShaders);

		CreateDescriptorSetLayout(descSetCreateInfo);
		CreateDescriptorPool(descSetCreateInfo);

		AllocateDescriptorSet(MAX_DESCRIPTOR_SET_COUNT / 2); // TODO: figure out the optimal allocation count in here
	}

	ShaderProgram_VK::ShaderProgram_VK(GraphicsHardwareInterface_VK* pDevice, LogicalDevice_VK* pLogicalDevice, const RawShader_VK* pVertexShader, const RawShader_VK* pFragmentShader)
		: ShaderProgram(0),
		m_pLogicalDevice(pLogicalDevice),
		m_descriptorSetAccessIndex(0)
	{
		m_pDevice = pDevice;

		m_shaderStages = 0;
		m_shaderStages |= (uint32_t)ShaderStageBitsConvert(VK_SHADER_STAGE_VERTEX_BIT);
		m_shaderStages |= (uint32_t)ShaderStageBitsConvert(VK_SHADER_STAGE_FRAGMENT_BIT);

		DescriptorSetCreateInfo descSetCreateInfo{};
		descSetCreateInfo.maxDescSetCount = MAX_DESCRIPTOR_SET_COUNT;

		ReflectResources(pVertexShader, descSetCreateInfo);

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = pVertexShader->m_shaderStage;
		shaderStageInfo.module = pVertexShader->m_shaderModule;
		shaderStageInfo.pName = pVertexShader->m_entryName;
		m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

		ReflectResources(pFragmentShader, descSetCreateInfo);

		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = pFragmentShader->m_shaderStage;
		shaderStageInfo.module = pFragmentShader->m_shaderModule;
		shaderStageInfo.pName = pFragmentShader->m_entryName;
		m_pipelineShaderStageCreateInfos.emplace_back(shaderStageInfo);

		CreateDescriptorSetLayout(descSetCreateInfo);
		CreateDescriptorPool(descSetCreateInfo);

		AllocateDescriptorSet(MAX_DESCRIPTOR_SET_COUNT / 2); // TODO: figure out the optimal allocation count in here
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
	}

	unsigned int ShaderProgram_VK::GetParamBinding(const char* paramName) const
	{
		if (m_resourceTable.find(paramName) != m_resourceTable.end())
		{
			return m_resourceTable.at(paramName).binding;
		}
		LOG_ERROR(std::string("Vulkan: Parameter name not found: ") + paramName);
		return -1;
	}

	void ShaderProgram_VK::Reset()
	{
		LOG_ERROR("Vulkan: shouldn't call Reset on Vulkan shader program.");
	}

	uint32_t ShaderProgram_VK::GetStageCount() const
	{
		return (uint32_t)m_pipelineShaderStageCreateInfos.size();
	}

	const VkPipelineShaderStageCreateInfo* ShaderProgram_VK::GetShaderStageCreateInfos() const
	{
		return m_pipelineShaderStageCreateInfos.data();
	}

	uint32_t ShaderProgram_VK::GetPushConstantRangeCount() const
	{
		return (uint32_t)m_pushConstantRanges.size();
	}

	const VkPushConstantRange* ShaderProgram_VK::GetPushConstantRanges() const
	{
		return m_pushConstantRanges.data();
	}

	DescriptorSet_VK* ShaderProgram_VK::GetDescriptorSet()
	{
		std::lock_guard<std::mutex> lock(m_descriptorSetGetMutex);

		bool flag = true;
		for (unsigned int i = m_descriptorSetAccessIndex; ; i = (i + 1) % m_descriptorSets.size())
		{
			if (!flag && i == m_descriptorSetAccessIndex)
			{
				// No available set found, allocate new one
				AllocateDescriptorSet(1); // ERROR: this would cause threading error if multiple threads are accessing the same descriptor pool

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

	void ShaderProgram_VK::ReflectResources(const RawShader_VK* pShader, DescriptorSetCreateInfo& descSetCreateInfo)
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
		LoadResourceDescriptor(spvCompiler, shaderRes, ShaderStageBitsConvert(pShader->m_shaderStage), descSetCreateInfo);
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

		uint32_t accumulatePushConstSize = 0;
		for (auto& constant : shaderRes.push_constant_buffers)
		{
			accumulatePushConstSize += (uint32_t)spvCompiler.get_declared_struct_size(spvCompiler.get_type(constant.base_type_id));

			ResourceDescription desc{};
			desc.type = EShaderResourceType_VK::PushConstant;
			desc.binding = spvCompiler.get_decoration(constant.id, spv::DecorationBinding);
			desc.name = MatchShaderParamName(spvCompiler.get_name(constant.id).c_str());

			m_resourceTable.emplace(desc.name, desc);
		}
		DEBUG_ASSERT_CE(accumulatePushConstSize < m_pLogicalDevice->deviceProperties.limits.maxPushConstantsSize);

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

	void ShaderProgram_VK::LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
	{
		// TODO: eliminate duplicate descriptor set create info

		LoadUniformBuffer(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
		LoadSeparateSampler(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
		LoadSeparateImage(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
		LoadImageSampler(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
		LoadPushConstantBuffer(spvCompiler, shaderRes, shaderType, m_pushConstantRanges);

		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs
	}

	void ShaderProgram_VK::LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
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

			if (descSetCreateInfo.recordedLayoutBindings.find(binding.binding) == descSetCreateInfo.recordedLayoutBindings.end())
			{
				descSetCreateInfo.recordedLayoutBindings.emplace(binding.binding, descSetCreateInfo.descSetLayoutBindings.size());
				descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else // Update stage flags
			{
				descSetCreateInfo.descSetLayoutBindings[descSetCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descSetCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) == descSetCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count;

				descSetCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = descSetCreateInfo.descSetPoolSizes.size(); // Record index
				descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descSetCreateInfo.descSetPoolSizes[descSetCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)].descriptorCount += descSetCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
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

			if (descSetCreateInfo.recordedLayoutBindings.find(binding.binding) == descSetCreateInfo.recordedLayoutBindings.end())
			{
				descSetCreateInfo.recordedLayoutBindings.emplace(binding.binding, descSetCreateInfo.descSetLayoutBindings.size());
				descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else
			{
				descSetCreateInfo.descSetLayoutBindings[descSetCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descSetCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_SAMPLER) == descSetCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
				poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count;

				descSetCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_SAMPLER] = descSetCreateInfo.descSetPoolSizes.size(); // Record index
				descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descSetCreateInfo.descSetPoolSizes[descSetCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_SAMPLER)].descriptorCount += descSetCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
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

			if (descSetCreateInfo.recordedLayoutBindings.find(binding.binding) == descSetCreateInfo.recordedLayoutBindings.end())
			{
				descSetCreateInfo.recordedLayoutBindings.emplace(binding.binding, descSetCreateInfo.descSetLayoutBindings.size());
				descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else
			{
				descSetCreateInfo.descSetLayoutBindings[descSetCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descSetCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) == descSetCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count;

				descSetCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] = descSetCreateInfo.descSetPoolSizes.size(); // Record index
				descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descSetCreateInfo.descSetPoolSizes[descSetCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)].descriptorCount += descSetCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadImageSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
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

			if (descSetCreateInfo.recordedLayoutBindings.find(binding.binding) == descSetCreateInfo.recordedLayoutBindings.end())
			{
				descSetCreateInfo.recordedLayoutBindings.emplace(binding.binding, descSetCreateInfo.descSetLayoutBindings.size());
				descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
				count++;
			}
			else // Update stage flags
			{
				descSetCreateInfo.descSetLayoutBindings[descSetCreateInfo.recordedLayoutBindings.at(binding.binding)].stageFlags |= binding.stageFlags;
			}
		}

		if (count > 0)
		{
			if (descSetCreateInfo.recordedPoolSizes.find(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) == descSetCreateInfo.recordedPoolSizes.end())
			{
				VkDescriptorPoolSize poolSize{};
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count;

				descSetCreateInfo.recordedPoolSizes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = descSetCreateInfo.descSetPoolSizes.size(); // Record index
				descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
			}
			else
			{
				descSetCreateInfo.descSetPoolSizes[descSetCreateInfo.recordedPoolSizes.at(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)].descriptorCount += descSetCreateInfo.maxDescSetCount * count;
			}
		}
	}

	void ShaderProgram_VK::LoadPushConstantBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, std::vector<VkPushConstantRange>& outRanges)
	{
		for (auto& constant : shaderRes.push_constant_buffers)
		{
			VkPushConstantRange range{};
			range.offset = spvCompiler.get_decoration(constant.id, spv::DecorationOffset);
			range.size = (uint32_t)spvCompiler.get_declared_struct_size(spvCompiler.get_type(constant.base_type_id));
			range.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the push constant is bind with multiple shader stages

			outRanges.emplace_back(range);
		}
	}

	void ShaderProgram_VK::CreateDescriptorSetLayout(const DescriptorSetCreateInfo& descSetCreateInfo)
	{
		CE_NEW(m_pDescriptorSetLayout, DescriptorSetLayout_VK, m_pLogicalDevice, descSetCreateInfo.descSetLayoutBindings);
	}

	void ShaderProgram_VK::CreateDescriptorPool(const DescriptorSetCreateInfo& descSetCreateInfo)
	{
		m_pDescriptorPool = m_pLogicalDevice->pDescriptorAllocator->CreateDescriptorPool(descSetCreateInfo.maxDescSetCount, descSetCreateInfo.descSetPoolSizes);
	}

	void ShaderProgram_VK::AllocateDescriptorSet(uint32_t count)
	{
		DEBUG_ASSERT_CE(m_pDescriptorPool);
		DEBUG_ASSERT_CE(m_descriptorSets.size() + count <= MAX_DESCRIPTOR_SET_COUNT);

		std::vector<VkDescriptorSetLayout> layouts(count, *m_pDescriptorSetLayout->GetDescriptorSetLayout());

		m_pDescriptorPool->AllocateDescriptorSets(layouts, m_descriptorSets);
	}

	void ShaderProgram_VK::UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_VK>& updateInfos)
	{
		m_pDescriptorPool->UpdateDescriptorSets(updateInfos);
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