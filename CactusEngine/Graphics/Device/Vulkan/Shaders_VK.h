#pragma once
#include "GraphicsResources.h"
#include "UploadAllocator_VK.h"
#include "DescriptorAllocator_VK.h"

#include <spirv_cross.hpp>
#include <unordered_map>

namespace Engine
{
	struct LogicalDevice_VK;
	class  GraphicsHardwareInterface_VK;

	class RawShader_VK
	{
	public:
		RawShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entry);
		~RawShader_VK();

	private:
		LogicalDevice_VK* m_pDevice;
		VkShaderModule			m_shaderModule;
		VkShaderStageFlagBits	m_shaderStage;
		const char* m_entryName;
		std::vector<char>		m_rawCode; // For reflection

		friend class ShaderProgram_VK;
		friend class VertexShader_VK;
		friend class FragmentShader_VK;
	};

	class VertexShader_VK : public VertexShader
	{
	public:
		VertexShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry = "main");
		~VertexShader_VK() = default;

		RawShader_VK* GetShaderImpl() const;

	private:
		RawShader_VK* m_pShaderImpl;
	};

	class FragmentShader_VK : public FragmentShader
	{
	public:
		FragmentShader_VK(LogicalDevice_VK* pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry = "main");
		~FragmentShader_VK() = default;

		RawShader_VK* GetShaderImpl() const;

	private:
		RawShader_VK* m_pShaderImpl;
	};

	enum class EShaderResourceType_VK
	{
		Uniform = 0,
		PushConstant,
		SeparateSampler,
		SeparateImage,
		SampledImage,
		StorageBuffer,
		StorageImage,
		SubpassInput,
		AccelerationStructure,
		COUNT
	};

	class ShaderProgram_VK : public ShaderProgram
	{
	public:
		ShaderProgram_VK(GraphicsHardwareInterface_VK* pDevice, LogicalDevice_VK* pLogicalDevice, uint32_t shaderCount, const RawShader_VK* pShader...); // Could also use a pointer array instead of variadic arguments
		ShaderProgram_VK(GraphicsHardwareInterface_VK* pDevice, LogicalDevice_VK* pLogicalDevice, const RawShader_VK* pVertexShader, const RawShader_VK* pFragmentShader);
		~ShaderProgram_VK();

		uint32_t GetParamBinding(const char* paramName) const override;
		void Reset() override; // Does not work on Vulkan shader program

		uint32_t GetStageCount() const;
		const VkPipelineShaderStageCreateInfo* GetShaderStageCreateInfos() const;

		uint32_t GetPushConstantRangeCount() const;
		const VkPushConstantRange* GetPushConstantRanges() const;

		DescriptorSet_VK* GetDescriptorSet();
		const DescriptorSetLayout_VK* GetDescriptorSetLayout() const;
		void UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_VK>& updateInfos);

	private:

		const uint32_t MAX_DESCRIPTOR_SET_COUNT = 1024; // TODO: figure out the proper value for this limit

		struct ResourceDescription
		{
			EShaderResourceType_VK	type;
			uint32_t				binding;
			const char* name;
		};

		struct DescriptorSetCreateInfo
		{
			std::vector<VkDescriptorSetLayoutBinding>	descSetLayoutBindings;
			std::vector<VkDescriptorPoolSize>			descSetPoolSizes;
			uint32_t									maxDescSetCount;

			// For duplicate removal
			std::unordered_map<uint32_t, uint32_t> recordedLayoutBindings; // binding - vector index
			std::unordered_map<VkDescriptorType, uint32_t> recordedPoolSizes; // type - vector index
		};

	private:
		// Shader reflection functions
		void ReflectResources(const RawShader_VK* pShader, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes);
		void LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadImageSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadPushConstantBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, std::vector<VkPushConstantRange>& outRanges);
		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs

		// Descriptor functions
		void CreateDescriptorSetLayout(const DescriptorSetCreateInfo& descSetCreateInfo);
		void CreateDescriptorPool(const DescriptorSetCreateInfo& descSetCreateInfo);
		void AllocateDescriptorSet(uint32_t count);

		// Converter functions
		uint32_t GetParamTypeSize(const spirv_cross::SPIRType& type);
		EDataType BasicTypeConvert(const spirv_cross::SPIRType& type);
		EShaderType ShaderStageBitsConvert(VkShaderStageFlagBits vkShaderStageBits);
		VkShaderStageFlagBits ShaderTypeConvertToStageBits(EShaderType shaderType);

	private:
		LogicalDevice_VK* m_pLogicalDevice;

		// Using char pointer as key would benefit runtime performance, but would reduce initialization speed as we need to match pointer by string contents
		std::unordered_map<const char*, ResourceDescription> m_resourceTable;
		std::vector<VkPushConstantRange> m_pushConstantRanges;

		DescriptorSetLayout_VK* m_pDescriptorSetLayout;
		DescriptorPool_VK* m_pDescriptorPool;
		std::vector<DescriptorSet_VK*> m_descriptorSets;
		uint32_t m_descriptorSetAccessIndex;
		mutable std::mutex m_descriptorSetGetMutex;

		std::vector<VkPipelineShaderStageCreateInfo> m_pipelineShaderStageCreateInfos;
	};
}