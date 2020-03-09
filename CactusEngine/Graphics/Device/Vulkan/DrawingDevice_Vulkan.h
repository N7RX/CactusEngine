#pragma once
#include "DrawingDevice.h"
#include "DrawingExtensionWrangler_Vulkan.h"
#include "DrawingCommandManager_Vulkan.h"
#include "DrawingResources_Vulkan.h"
#include "DrawingUploadAllocator_Vulkan.h"
#include "DrawingDescriptorAllocator_Vulkan.h"

namespace Engine
{
	struct LogicalDevice_Vulkan
	{
		EGPUType				   type;
		VkPhysicalDevice		   physicalDevice;
		VkDevice				   logicalDevice;
		VkPhysicalDeviceProperties deviceProperties;

		DrawingCommandQueue_Vulkan presentQueue;
		DrawingCommandQueue_Vulkan graphicsQueue;

		std::shared_ptr<DrawingCommandManager_Vulkan> pGraphicsCommandManager;
#if defined(ENABLE_COPY_QUEUE_VK)
		DrawingCommandQueue_Vulkan copyQueue;
		std::shared_ptr<DrawingCommandManager_Vulkan> pCopyCommandManager;
#endif
		std::shared_ptr<DrawingUploadAllocator_Vulkan>		pUploadAllocator;
		std::shared_ptr<DrawingDescriptorAllocator_Vulkan>	pDescriptorAllocator;
		std::shared_ptr<DrawingSyncObjectManager_Vulkan>	pSyncObjectManager;

		std::shared_ptr<DrawingCommandBuffer_Vulkan>		pImplicitCmdBuffer; // Command buffer used implicitly inside drawing device
	};

	class DrawingDevice_Vulkan : public DrawingDevice
	{
	public:
		const std::vector<const char*> m_validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> m_deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const VkPhysicalDeviceFeatures m_deviceFeatures =
		{
			// TODO: List required features
		};

	public:
		~DrawingDevice_Vulkan();

		void Initialize() override;
		void ShutDown() override;

		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;
		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) override;
		bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput) override;
		bool CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput) override;

		void ClearRenderTarget() override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments) override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer) override;
		void SetClearColor(Color4 color) override;
		void SetBlendState(const DeviceBlendStateInfo& blendInfo) override;

		void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void DrawFullScreenQuad(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		EGraphicsDeviceType GetDeviceType() const override;

		// Low-level functions exclusive to Vulkan device
		void SetupDevice();
		std::shared_ptr<LogicalDevice_Vulkan> GetLogicalDevice(EGPUType type) const;

		std::shared_ptr<DrawingCommandPool> RequestExternalCommandPool(EGPUType deviceType) override;
		std::shared_ptr<DrawingCommandBuffer> RequestCommandBuffer(std::shared_ptr<DrawingCommandPool> pCommandPool) override;
		void ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;

		bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput) override;
		bool CreateSampler(const TextureSamplerCreateInfo& createInfo, std::shared_ptr<TextureSampler>& pOutput) override;
		bool CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, std::shared_ptr<PipelineVertexInputState>& pOutput) override;
		bool CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, std::shared_ptr<PipelineInputAssemblyState>& pOutput) override;
		bool CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, std::shared_ptr<PipelineColorBlendState>& pOutput) override;
		bool CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, std::shared_ptr<PipelineRasterizationState>& pOutput) override;
		bool CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, std::shared_ptr<PipelineDepthStencilState>& pOutput) override;
		bool CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, std::shared_ptr<PipelineMultisampleState>& pOutput) override;
		bool CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, std::shared_ptr<PipelineViewportState>& pOutput) override;
		bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput) override;

		void TransitionImageLayout(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void TransitionImageLayout_Immediate(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void ResizeSwapchain(uint32_t width, uint32_t height) override;

		void BindGraphicsPipeline(const std::shared_ptr<GraphicsPipelineObject> pPipeline, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void EndRenderPass(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void EndCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;

		void Present() override;	
		void FlushCommands(bool waitExecution, bool flushImplicitCommands) override;

		std::shared_ptr<TextureSampler> GetDefaultTextureSampler(EGPUType deviceType) const override;
		void GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const override;
		uint32_t GetSwapchainPresentImageIndex() const override;

		void ConfigureStates_Test() override;

	private:
		// Initialization functions
		void GetRequiredExtensions();
		void CreateInstance();
		void SetupDebugMessenger();
		void CreatePresentationSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateLogicalDevice(std::shared_ptr<LogicalDevice_Vulkan> pDevice);
		void SetupSwapchain();
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void CreateDefaultSampler();

		// Shader-related functions
		void CreateShaderModuleFromFile(const char* shaderFilePath, std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode);

		// Manager setup functions
		void SetupCommandManager();
		void SetupSyncObjectManager();
		void SetupUploadAllocator();
		void SetupDescriptorAllocator();

		// Converter functions
		EDescriptorResourceType_Vulkan VulkanDescriptorResourceType(EDescriptorType type) const;
		void GetBufferInfoByDescriptorType(EDescriptorType type, const std::shared_ptr<RawResource> pRes, VkDescriptorBufferInfo& outInfo);

	public:
		const uint64_t FRAME_TIMEOUT = 5e9; // 5 seconds
		const static unsigned int MAX_FRAME_IN_FLIGHT = 2;

	private:

#if defined(_DEBUG)
		const bool m_enableValidationLayers = true;
#else
		const bool m_enableValidationLayers = false;
#endif

	private:
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice_0; // Discrete GPU
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice_1; // Integrated GPU
#endif		
		VkInstance m_instance;
		VkSurfaceKHR m_presentationSurface;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		bool m_isRunning;
		
		VkApplicationInfo m_appInfo;
		std::vector<const char*> m_requiredExtensions;
		std::vector<VkExtensionProperties> m_availableExtensions;

		std::shared_ptr<DrawingSwapchain_Vulkan> m_pSwapchain;
		unsigned int m_currentFrame;

		std::shared_ptr<Sampler_Vulkan> m_pDefaultSampler_0;
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
		std::shared_ptr<Sampler_Vulkan> m_pDefaultSampler_1; // For integrated GPU
#endif
	};

	template<>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice<EGraphicsDeviceType::Vulkan>()
	{
		auto pDevice = std::make_shared<DrawingDevice_Vulkan>();
		pDevice->SetupDevice();

		return pDevice;
	}
}