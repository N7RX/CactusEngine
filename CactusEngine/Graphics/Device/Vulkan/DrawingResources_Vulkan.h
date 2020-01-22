#pragma once
#include "DrawingResources.h"
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingUtil_Vulkan.h"
#include "DrawingExtensionWrangler_Vulkan.h"
#include "DrawingSyncObjectManager_Vulkan.h"

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
		VK,
		VMA,
		COUNT
	};

	class Texture2D_Vulkan : public Texture2D
	{
	public:
		Texture2D_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const Texture2DCreateInfo_Vulkan& createInfo);
		virtual ~Texture2D_Vulkan();

		uint32_t GetTextureID() const override;

	protected:
		Texture2D_Vulkan() = default;

	protected:
		std::shared_ptr<DrawingDevice_Vulkan> m_pDrawingDevice;
		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;

		VkImage			m_image = VK_NULL_HANDLE;
		VkImageView		m_imageView = VK_NULL_HANDLE;
		VkImageLayout	m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocation	m_allocation = VK_NULL_HANDLE;
		VkExtent2D		m_extent = { 0, 0 };
		VkFormat		m_format = VK_FORMAT_UNDEFINED;
		uint32_t		m_mipLevels = 0;

		EAllocatorType_Vulkan m_allocatorType;

		friend class DrawingDevice_Vulkan;
		friend class DrawingCommandBuffer_Vulkan;
		friend class DrawingUploadAllocator_Vulkan;
	};

	class RenderTarget2D_Vulkan : public Texture2D_Vulkan
	{
	public:
		RenderTarget2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat);
		~RenderTarget2D_Vulkan();
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

	class FrameBuffer_Vulkan : public FrameBuffer
	{
	public:
		~FrameBuffer_Vulkan();

		uint32_t GetFrameBufferID() const override;
		// TODO: finish constructing Vulkan framebuffer wrapper class

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice;
		VkFramebuffer m_frameBuffer;
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
		DrawingSwapchain_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const DrawingSwapchainCreateInfo_Vulkan& createInfo);
		~DrawingSwapchain_Vulkan();

		bool UpdateBackBuffer(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore, uint64_t timeout);
		bool Present(const std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>>& waitSemaphores, uint32_t syncInterval);

		uint32_t GetTargetCount() const;
		std::shared_ptr<FrameBuffer_Vulkan> GetTargetFrameBuffer() const;

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;
		VkSwapchainKHR m_swapchain;
		VkQueue m_presentQueue;

		std::vector<std::shared_ptr<RenderTarget2D_Vulkan>> m_renderTargets;
		std::vector<std::shared_ptr<FrameBuffer_Vulkan>> m_swapchainFrameBuffers;
		uint32_t m_frameBufferIndex;
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

	private:
		// Shader reflection functions
		void ReflectResources(const std::shared_ptr<RawShader_Vulkan> pShader);
		void ProcessVariables(const spirv_cross::Compiler& spvCompiler, const spirv_cross::Resource& resource);
		void LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes);
		void LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetsCount);
		void LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetsCount);
		void LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetsCount);
		void LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetsCount);
		void LoadSampledImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetsCount);
		void LoadPushConstantBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType);
		// TODO: handle storage buffers
		// TODO: handle storage textures
		// TODO: handle subpass inputs
		// TODO: handle acceleration structures

		// Converter functions
		EShaderParamType GetParamType(const spirv_cross::SPIRType& type, uint32_t size);
		uint32_t GetParamTypeSize(const spirv_cross::SPIRType& type);
		EDataType BasicTypeConvert(const spirv_cross::SPIRType& type);
		EShaderType ShaderStageBitsConvert(VkShaderStageFlagBits vkShaderStageBits);

		// Descriptor functions


	private:
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

		std::shared_ptr<LogicalDevice_Vulkan> m_pLogicalDevice;
		std::unordered_map<const char*, ResourceDescription> m_resourceTable;
		std::unordered_map<const char*, VariableDescription> m_variableTable;
	};
}