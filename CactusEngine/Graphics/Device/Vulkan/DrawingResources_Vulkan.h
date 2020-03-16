#pragma once
#include "DrawingResources.h"
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingUtil_Vulkan.h"
#include "DrawingExtensionWrangler_Vulkan.h"
#include "DrawingSyncObjectManager_Vulkan.h"
#include "DrawingDescriptorAllocator_Vulkan.h"

#include <spirv_cross.hpp>
#include <unordered_map>

namespace Engine
{
	struct LogicalDevice_Vulkan;
	class DrawingDevice_Vulkan;
	class DrawingCommandBuffer_Vulkan;	

	enum class EAllocatorType_Vulkan
	{
		None = 0,
		VK,  // Native Vulkan allocation
		VMA, // Managed by Vulkan Memory Allocator (VMA) library
		COUNT
	};

	class RawBuffer_Vulkan : public RawResource
	{
	public:
		RawBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& createInfo);
		virtual ~RawBuffer_Vulkan();

	protected:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;

		VkBuffer		m_buffer;
		VmaAllocation	m_allocation;
		VkDeviceSize	m_deviceSize;

		friend class DrawingCommandBuffer_Vulkan;
		friend class DrawingUploadAllocator_Vulkan;
		friend class DrawingDevice_Vulkan;
		friend class UniformBuffer_Vulkan;
		friend class DataTransferBuffer_Vulkan;
	};

	class DataTransferBuffer_Vulkan : public DataTransferBuffer
	{
	public:
		DataTransferBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& createInfo);
		~DataTransferBuffer_Vulkan();

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<RawBuffer_Vulkan> m_pBufferImpl;

		bool m_constantlyMapped;
		void* m_ppMappedData;

		friend class DrawingDevice_Vulkan;
	};

	class VertexBuffer_Vulkan : public VertexBuffer
	{
	public:
		VertexBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& vertexBufferCreateInfo, const RawBufferCreateInfo_Vulkan& indexBufferCreateInfo);
		~VertexBuffer_Vulkan() = default;

		std::shared_ptr<RawBuffer_Vulkan> GetBufferImpl() const;
		std::shared_ptr<RawBuffer_Vulkan> GetIndexBufferImpl() const;
		VkIndexType GetIndexFormat() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<RawBuffer_Vulkan> m_pVertexBufferImpl;
		std::shared_ptr<RawBuffer_Vulkan> m_pIndexBufferImpl;
		VkIndexType m_indexType;
	};

	class Sampler_Vulkan : public TextureSampler
	{
	public:
		Sampler_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const VkSamplerCreateInfo& createInfo);
		~Sampler_Vulkan();

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkSampler m_sampler;

		friend class DrawingDevice_Vulkan;
		friend class Texture2D_Vulkan;
	};

	class Texture2D_Vulkan : public Texture2D
	{
	public:
		Texture2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const Texture2DCreateInfo_Vulkan& createInfo);
		virtual ~Texture2D_Vulkan();

		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	protected:
		Texture2D_Vulkan();

	protected:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;

		VkImage				m_image = VK_NULL_HANDLE;
		VkImageView			m_imageView = VK_NULL_HANDLE;
		VkImageLayout		m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocation		m_allocation = VK_NULL_HANDLE;
		VkExtent2D			m_extent = { 0, 0 };
		VkFormat			m_format = VK_FORMAT_UNDEFINED;
		uint32_t			m_mipLevels = 0;
		VkImageAspectFlags	m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		EAllocatorType_Vulkan m_allocatorType;

		std::shared_ptr<Sampler_Vulkan> m_pSampler;

		uint32_t m_appliedStages; // EShaderType bitmap

		friend class DrawingDevice_Vulkan;
		friend class DrawingCommandBuffer_Vulkan;
		friend class DrawingUploadAllocator_Vulkan;
	};

	class RenderTarget2D_Vulkan : public Texture2D_Vulkan
	{
	public:
		RenderTarget2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat);
		~RenderTarget2D_Vulkan();
	};

	enum class EUniformBufferType_Vulkan
	{
		Undefined = 0,
		Uniform,
		PushConstant,
		COUNT
	};

	struct UniformBufferCreateInfo_Vulkan
	{
		EUniformBufferType_Vulkan	type;
		uint32_t					size;
		VkShaderStageFlags			appliedStages;
	};

	class UniformBuffer_Vulkan : public UniformBuffer
	{
	public:
		UniformBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const UniformBufferCreateInfo_Vulkan& createInfo);
		~UniformBuffer_Vulkan();

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;
		std::shared_ptr<SubUniformBuffer> AllocateSubBuffer(uint32_t size) override;
		void ResetSubBufferAllocation() override;

		void UpdateToDevice(std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer = nullptr);
		std::shared_ptr<RawBuffer_Vulkan> GetBufferImpl() const;

		EUniformBufferType_Vulkan GetType() const;

	private:
		std::shared_ptr<RawBuffer_Vulkan> m_pBufferImpl;
		EUniformBufferType_Vulkan m_eType;
		VkShaderStageFlags m_appliedShaderStage;

		const void* m_pRawData;
		void* m_pHostData; // Pointer to mapped host memory location

		uint32_t m_subAllocatedSize;
		mutable std::mutex m_subAllocateMutex;

		friend class SubUniformBuffer_Vulkan;
	};

	class SubUniformBuffer_Vulkan : public SubUniformBuffer
	{
	public:
		SubUniformBuffer_Vulkan(UniformBuffer_Vulkan* pParentBuffer, VkBuffer buffer, uint32_t offset, uint32_t size);
		~SubUniformBuffer_Vulkan() = default;

		void UpdateSubBufferData(const void* pData) override;

	private:
		UniformBuffer_Vulkan* m_pParentBuffer;
		VkBuffer m_buffer;
		uint32_t m_offset;
		uint32_t m_size;

		friend class DrawingDevice_Vulkan;
		friend class UniformBuffer_Vulkan;
	};

	class RenderPass_Vulkan : public RenderPassObject
	{
	public:
		RenderPass_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~RenderPass_Vulkan();

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkRenderPass m_renderPass;
		std::vector<VkClearValue> m_clearValues;

		friend class DrawingDevice_Vulkan;
	};

	class FrameBuffer_Vulkan : public FrameBuffer
	{
	public:
		FrameBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~FrameBuffer_Vulkan();

		uint32_t GetFrameBufferID() const override;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkFramebuffer m_frameBuffer;
		std::vector<std::shared_ptr<Texture2D_Vulkan>> m_bufferAttachments;

		friend class DrawingDevice_Vulkan;
	};

	struct DrawingSwapchainCreateInfo_Vulkan
	{
		VkSurfaceKHR				surface;
		VkSurfaceFormatKHR			surfaceFormat;
		VkPresentModeKHR			presentMode;
		VkExtent2D					swapExtent;
		SwapchainSupportDetails_VK	supportDetails;
		QueueFamilyIndices_VK		queueFamilyIndices;
		uint32_t					maxFramesInFlight;
	};

	class DrawingSwapchain_Vulkan
	{
	public:
		DrawingSwapchain_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingSwapchainCreateInfo_Vulkan& createInfo);
		~DrawingSwapchain_Vulkan();

		bool UpdateBackBuffer(unsigned int currentFrame);
		bool Present(const std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>>& waitSemaphores);

		uint32_t GetSwapchainImageCount() const;
		std::shared_ptr<RenderTarget2D_Vulkan> GetTargetImage() const;
		uint32_t GetTargetImageIndex() const;
		std::shared_ptr<RenderTarget2D_Vulkan> GetSwapchainImageByIndex(unsigned int index) const;
		VkExtent2D GetSwapExtent() const;
		std::shared_ptr<DrawingSemaphore_Vulkan> GetImageAvailableSemaphore(unsigned int currentFrame) const;

	public:
		const uint64_t ACQUIRE_IMAGE_TIMEOUT = 3e9; // 3 seconds

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkSwapchainKHR m_swapchain;
		VkQueue m_presentQueue;
		VkExtent2D m_swapExtent;

		std::vector<std::shared_ptr<RenderTarget2D_Vulkan>> m_renderTargets; // Swapchain images
		uint32_t m_targetImageIndex;
		std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> m_imageAvailableSemaphores;

		friend class DrawingDevice_Vulkan;
	};

	class RawShader_Vulkan
	{
	public:
		RawShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entry);
		~RawShader_Vulkan();

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkShaderModule			m_shaderModule;
		VkShaderStageFlagBits	m_shaderStage;
		const char*				m_entryName;
		std::vector<char>		m_rawCode; // For reflection

		friend class ShaderProgram_Vulkan;
		friend class VertexShader_Vulkan;
		friend class FragmentShader_Vulkan;	
	};

	class VertexShader_Vulkan : public VertexShader
	{
	public:
		VertexShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry = "main");
		~VertexShader_Vulkan() = default;

		std::shared_ptr<RawShader_Vulkan> GetShaderImpl() const;

	private:
		std::shared_ptr<RawShader_Vulkan> m_pShaderImpl;
	};

	class FragmentShader_Vulkan : public FragmentShader
	{
	public:
		FragmentShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry = "main");
		~FragmentShader_Vulkan() = default;

		std::shared_ptr<RawShader_Vulkan> GetShaderImpl() const;

	private:
		std::shared_ptr<RawShader_Vulkan> m_pShaderImpl;
	};

	enum class EShaderResourceType_Vulkan
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

	class ShaderProgram_Vulkan : public ShaderProgram
	{
	public:
		ShaderProgram_Vulkan(DrawingDevice_Vulkan* pDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, uint32_t shaderCount, const std::shared_ptr<RawShader_Vulkan> pShader...); // Could also use a pointer array instead of variadic arguments
		ShaderProgram_Vulkan(DrawingDevice_Vulkan* pDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const std::shared_ptr<RawShader_Vulkan> pVertexShader, const std::shared_ptr<RawShader_Vulkan> pFragmentShader);
		~ShaderProgram_Vulkan();

		unsigned int GetParamBinding(const char* paramName) const override;
		void Reset() override; // Does not work on Vulkan shader program

		uint32_t GetStageCount() const;
		const VkPipelineShaderStageCreateInfo* GetShaderStageCreateInfos() const;

		uint32_t GetPushConstantRangeCount() const;
		const VkPushConstantRange* GetPushConstantRanges() const;

		std::shared_ptr<DrawingDescriptorSet_Vulkan> GetDescriptorSet();
		const DrawingDescriptorSetLayout_Vulkan* GetDescriptorSetLayout() const;
		void UpdateDescriptorSets(const std::vector<DesciptorUpdateInfo_Vulkan>& updateInfos);

	private:

		const uint32_t MAX_DESCRIPTOR_SET_COUNT = 1024; // TODO: figure out the proper value for this limit

		struct ResourceDescription
		{
			EShaderResourceType_Vulkan	type;
			unsigned int				binding;
			const char*					name;
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
		void ReflectResources(const std::shared_ptr<RawShader_Vulkan> pShader, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes);
		void LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType,  DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadImageSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo);
		void LoadPushConstantBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, std::vector<VkPushConstantRange>& outRanges);
		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs
		// TODO: handle acceleration structures

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
		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;

		// Using char pointer as key would benefit runtime performance, but would reduce initialization speed as we need to match pointer by string contents
		std::unordered_map<const char*, ResourceDescription> m_resourceTable;
		std::vector<VkPushConstantRange> m_pushConstantRanges;

		std::shared_ptr<DrawingDescriptorSetLayout_Vulkan> m_pDescriptorSetLayout;
		std::shared_ptr<DrawingDescriptorPool_Vulkan> m_pDescriptorPool;
		std::vector<std::shared_ptr<DrawingDescriptorSet_Vulkan>> m_descriptorSets;
		unsigned int m_descriptorSetAccessIndex;
		mutable std::mutex m_descriptorSetGetMutex;

		std::vector<VkPipelineShaderStageCreateInfo> m_pipelineShaderStageCreateInfos;
	};

	class GraphicsPipeline_Vulkan : public GraphicsPipelineObject
	{
	public:
		GraphicsPipeline_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::shared_ptr<ShaderProgram_Vulkan> pShaderProgram, VkGraphicsPipelineCreateInfo& createInfo);
		~GraphicsPipeline_Vulkan();

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;
		VkPipelineBindPoint GetBindPoint() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<ShaderProgram_Vulkan> m_pShaderProgram;

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};

	class PipelineVertexInputState_Vulkan : public PipelineVertexInputState
	{
	public:
		PipelineVertexInputState_Vulkan(const std::vector<VkVertexInputBindingDescription>& bindingDescs, const std::vector<VkVertexInputAttributeDescription>& attributeDescs);
		~PipelineVertexInputState_Vulkan() = default;

		const VkPipelineVertexInputStateCreateInfo* GetVertexInputStateCreateInfo() const;

	private:
		std::vector<VkVertexInputBindingDescription>	m_vertexBindingDesc;
		std::vector<VkVertexInputAttributeDescription>	m_vertexAttributeDesc;
		VkPipelineVertexInputStateCreateInfo			m_pipelineVertexInputStateCreateInfo;
	};

	class PipelineInputAssemblyState_Vulkan : public PipelineInputAssemblyState
	{
	public:
		PipelineInputAssemblyState_Vulkan(const PipelineInputAssemblyStateCreateInfo& createInfo);
		~PipelineInputAssemblyState_Vulkan() = default;

		const VkPipelineInputAssemblyStateCreateInfo* GetInputAssemblyStateCreateInfo() const;

	private:
		VkPipelineInputAssemblyStateCreateInfo m_pipelineInputAssemblyStateCreateInfo;
	};

	class PipelineColorBlendState_Vulkan : public PipelineColorBlendState
	{
	public:
		PipelineColorBlendState_Vulkan(const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachmentStates);
		~PipelineColorBlendState_Vulkan() = default;

		const VkPipelineColorBlendStateCreateInfo* GetColorBlendStateCreateInfo() const;

		void EnableLogicOperation();
		void DisableLogicOpeartion();
		void SetLogicOpeartion(VkLogicOp logicOp);

	private:
		std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;
		VkPipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo;
	};

	class PipelineRasterizationState_Vulkan : public PipelineRasterizationState
	{
	public:
		PipelineRasterizationState_Vulkan(const PipelineRasterizationStateCreateInfo& createInfo);
		~PipelineRasterizationState_Vulkan() = default;

		const VkPipelineRasterizationStateCreateInfo* GetRasterizationStateCreateInfo() const;

	private:
		VkPipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo;
	};

	class PipelineDepthStencilState_Vulkan : public PipelineDepthStencilState
	{
	public:
		PipelineDepthStencilState_Vulkan(const PipelineDepthStencilStateCreateInfo& createInfo);
		~PipelineDepthStencilState_Vulkan() = default;

		const VkPipelineDepthStencilStateCreateInfo* GetDepthStencilStateCreateInfo() const;

	private:
		VkPipelineDepthStencilStateCreateInfo m_pipelineDepthStencilStateCreateInfo;
	};

	class PipelineMultisampleState_Vulkan : public PipelineMultisampleState
	{
	public:
		PipelineMultisampleState_Vulkan(const PipelineMultisampleStateCreateInfo& createInfo);
		~PipelineMultisampleState_Vulkan() = default;

		const VkPipelineMultisampleStateCreateInfo* GetMultisampleStateCreateInfo() const;

	private:
		VkPipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo;
	};

	class PipelineViewportState_Vulkan : public PipelineViewportState
	{
	public:
		PipelineViewportState_Vulkan(const PipelineViewportStateCreateInfo& createInfo);
		~PipelineViewportState_Vulkan() = default;

		const VkPipelineViewportStateCreateInfo* GetViewportStateCreateInfo() const;

	private:
		// Currently only one viewport is supported
		VkViewport m_viewport;
		VkRect2D m_scissor;
		VkPipelineViewportStateCreateInfo m_pipelineViewportStateCreateInfo;
	};
}