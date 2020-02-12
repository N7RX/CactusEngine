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
		VK, // Native Vulkan allocation
		VMA,// Managed by Vulkan Memory Allocator library
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

	class Texture2D_Vulkan : public Texture2D
	{
	public:
		Texture2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const Texture2DCreateInfo_Vulkan& createInfo);
		virtual ~Texture2D_Vulkan();

		uint32_t GetTextureID() const override { return -1; };// This is currently not used in Vulkan device

	protected:
		Texture2D_Vulkan() = default;

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
		EUniformBufferType_Vulkan type;
		uint32_t size;
		uint32_t layoutParamIndex;
		VkShaderStageFlags appliedStages;
	};

	class UniformBuffer_Vulkan : public RawBuffer_Vulkan
	{
	public:
		UniformBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const UniformBufferCreateInfo_Vulkan& createInfo);
		~UniformBuffer_Vulkan();

		void UpdateBufferData(const std::shared_ptr<void> pData);
		void UpdateToDevice(std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer = nullptr);

		EUniformBufferType_Vulkan GetType() const;
		uint32_t GetBindingIndex() const;

	private:
		uint32_t m_layoutParamIndex;
		EUniformBufferType_Vulkan m_eType;
		VkShaderStageFlags m_appliedShaderStage;

		bool m_memoryMapped;
		void* m_pHostData; // Pointer to mapped host memory location	
	};

	class RenderPass_Vulkan : public RenderPassObject
	{

	private:
		VkRenderPass m_renderPass;

		friend class DrawingDevice_Vulkan;
	};

	class FrameBuffer_Vulkan : public FrameBuffer
	{
	public:
		FrameBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		~FrameBuffer_Vulkan();

		uint32_t GetFrameBufferID() const override;
		// TODO: finish constructing Vulkan framebuffer wrapper class

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

		bool UpdateBackBuffer(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore, uint64_t timeout);
		bool Present(const std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>>& waitSemaphores, uint32_t syncInterval);

		uint32_t GetSwapchainImageCount() const;
		std::shared_ptr<RenderTarget2D_Vulkan> GetTargetImage() const;
		VkExtent2D GetSwapExtent() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkSwapchainKHR m_swapchain;
		VkQueue m_presentQueue;
		VkExtent2D m_swapExtent;

		std::vector<std::shared_ptr<RenderTarget2D_Vulkan>> m_renderTargets; // Swapchain images
		uint32_t m_targetImageIndex;
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
		~ShaderProgram_Vulkan() = default;

	public:
		void Reset() override {};

	private:
		const float MAX_DESCRIPTOR_SET_COUNT = 32;

		struct ResourceDescription
		{
			EShaderResourceType_Vulkan type;
			unsigned int slot;
			const char* name;
		};

		struct VariableDescription
		{
			const char* uniformName;
			const char* variableName;
			uint32_t offset;
			uint32_t uniformSize;
			uint32_t variableSize;
			EShaderParamType paramType;
		};

		struct DescriptorSetCreateInfo
		{
			std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBindings;
			std::vector<VkDescriptorPoolSize> descSetPoolSizes;
			uint32_t maxDescSetCount;
		};

	private:
		// Shader reflection functions
		void ReflectResources(const std::shared_ptr<RawShader_Vulkan> pShader);
		void ProcessVariables(const spirv_cross::Compiler& spvCompiler, const spirv_cross::Resource& resource);
		void LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes);
		void LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetCount);
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
		EShaderParamType GetParamType(const spirv_cross::SPIRType& type, uint32_t size);
		uint32_t GetParamTypeSize(const spirv_cross::SPIRType& type);
		EDataType BasicTypeConvert(const spirv_cross::SPIRType& type);
		EShaderType ShaderStageBitsConvert(VkShaderStageFlagBits vkShaderStageBits);
		VkShaderStageFlagBits ShaderTypeConvertToStageBits(EShaderType shaderType);	


	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;
		std::unordered_map<const char*, ResourceDescription> m_resourceTable;
		std::unordered_map<const char*, VariableDescription> m_variableTable;
		std::shared_ptr<DrawingDescriptorSetLayout_Vulkan> m_pDescriptorSetLayout;
		std::shared_ptr<DrawingDescriptorPool_Vulkan> m_pDescriptorPool;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};

	class GraphicsPipeline_Vulkan : public GraphicsPipelineObject
	{
	public:
		GraphicsPipeline_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::shared_ptr<ShaderProgram_Vulkan> pShaderProgram, VkGraphicsPipelineCreateInfo& createInfo);
		~GraphicsPipeline_Vulkan();

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		std::shared_ptr<ShaderProgram_Vulkan> m_pShaderProgram;

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;
	};

	class PipelineVertexInputState_Vulkan
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

	class PipelineColorBlendState_Vulkan
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

	class PipelineViewportState_Vulkan
	{
	public:
		PipelineViewportState_Vulkan(uint32_t width, uint32_t height);
		~PipelineViewportState_Vulkan() = default;

		const VkPipelineViewportStateCreateInfo* GetViewportStateCreateInfo() const;

	private:
		// Currently only one viewport is supported
		VkViewport m_viewport;
		VkRect2D m_scissor;
		VkPipelineViewportStateCreateInfo m_pipelineViewportStateCreateInfo;
	};
}