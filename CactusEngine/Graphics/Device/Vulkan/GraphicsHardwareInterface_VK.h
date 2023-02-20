#pragma once
#include "GraphicsDevice.h"
#include "ExtensionLoader_VK.h"
#include "CommandManager_VK.h"
#include "UploadAllocator_VK.h"
#include "DescriptorAllocator_VK.h"

namespace Engine
{
	class Sampler_VK;
	class Swapchain_VK;

	struct LogicalDevice_VK
	{
		EGPUType				   type;
		VkPhysicalDevice		   physicalDevice;
		VkDevice				   logicalDevice;
		VkPhysicalDeviceProperties deviceProperties;

		CommandQueue_VK presentQueue;
		CommandQueue_VK graphicsQueue;
		CommandQueue_VK transferQueue;

		std::shared_ptr<CommandManager_VK>		pGraphicsCommandManager;
		std::shared_ptr<CommandManager_VK>		pTransferCommandManager;
		std::shared_ptr<UploadAllocator_VK>		pUploadAllocator;
		std::shared_ptr<DescriptorAllocator_VK>	pDescriptorAllocator;
		std::shared_ptr<SyncObjectManager_VK>	pSyncObjectManager;

		std::shared_ptr<CommandBuffer_VK>		pImplicitCmdBuffer; // Command buffer used implicitly inside drawing device, for graphics queue
	};

	class GraphicsHardwareInterface_VK : public GraphicsDevice
	{
	public:
		const std::vector<const char*> m_validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> m_deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
		};

	public:
		~GraphicsHardwareInterface_VK();

		void Initialize() override;
		void ShutDown() override;

		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;
		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) override;
		bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput) override;
		bool CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput) override;

		void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) override;
		void DrawFullScreenQuad(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		EGraphicsAPIType GetDeviceType() const override;

		// Low-level functions exclusive to Vulkan device
		void SetupDevice();
		std::shared_ptr<LogicalDevice_VK> GetLogicalDevice(EGPUType type) const;

		std::shared_ptr<DrawingCommandPool> RequestExternalCommandPool(EQueueType queueType, EGPUType deviceType = EGPUType::Main) override;
		std::shared_ptr<DrawingCommandBuffer> RequestCommandBuffer(std::shared_ptr<DrawingCommandPool> pCommandPool) override;
		void ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		std::shared_ptr<DrawingSemaphore> RequestDrawingSemaphore(EGPUType deviceType, ESemaphoreWaitStage waitStage) override;

		bool CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, std::shared_ptr<DataTransferBuffer>& pOutput) override;
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
		void CommandWaitSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore) override;
		void CommandSignalSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore) override;

		void Present() override;	
		void FlushCommands(bool waitExecution, bool flushImplicitCommands, uint32_t deviceTypeFlags = (uint32_t)EGPUType::Main | (uint32_t)EGPUType::Secondary) override;
		void FlushTransferCommands(bool waitExecution) override;
		void WaitSemaphore(std::shared_ptr<DrawingSemaphore> pSemaphore) override;

		std::shared_ptr<TextureSampler> GetDefaultTextureSampler(EGPUType deviceType = EGPUType::Main, bool withDefaultAF = false) const override;
		void GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const override;
		uint32_t GetSwapchainPresentImageIndex() const override;

		void CopyTexture2DToDataTransferBuffer(std::shared_ptr<Texture2D> pSrcTexture, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void CopyDataTransferBufferToTexture2D(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<Texture2D> pDstTexture, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void CopyDataTransferBufferCrossDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer) override;
		void CopyDataTransferBufferWithinDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) override;
		void CopyHostDataToDataTransferBuffer(void* pData, std::shared_ptr<DataTransferBuffer> pDstBuffer, size_t size) override;
		void CopyDataTransferBufferToHostDataLocation(std::shared_ptr<DataTransferBuffer> pSrcBuffer, void* pDataLoc) override;

	private:
		// Initialization functions
		void GetRequiredExtensions();
		void CreateInstance();
		void SetupDebugMessenger();
		void CreatePresentationSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateLogicalDevice(std::shared_ptr<LogicalDevice_VK> pDevice);
		void SetupSwapchain();
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void CreateDefaultSampler();

		// Shader-related functions
		void CreateShaderModuleFromFile(const char* shaderFilePath, std::shared_ptr<LogicalDevice_VK> pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode);

		// Manager setup functions
		void SetupCommandManager();
		void SetupSyncObjectManager();
		void SetupUploadAllocator();
		void SetupDescriptorAllocator();

		// Converter functions
		EDescriptorResourceType_VK VulkanDescriptorResourceType(EDescriptorType type) const;
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
		VkInstance m_instance;
		VkSurfaceKHR m_presentationSurface;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		bool m_isRunning;
		
		VkApplicationInfo m_appInfo;
		std::vector<const char*> m_requiredExtensions;
		std::vector<VkExtensionProperties> m_availableExtensions;

		std::shared_ptr<LogicalDevice_VK> m_pDevice_0; // Main GPU
		//... (Extend required GPU here)

		std::shared_ptr<Swapchain_VK> m_pSwapchain;
		unsigned int m_currentFrame;

		std::shared_ptr<Sampler_VK> m_pDefaultSampler_0; // For main GPU
		std::shared_ptr<Sampler_VK> m_pDefaultSampler_1;
		//... (Extend required GPU here)
	};

	template<>
	static std::shared_ptr<GraphicsDevice> CreateDrawingDevice<EGraphicsAPIType::Vulkan>()
	{
		auto pDevice = std::make_shared<GraphicsHardwareInterface_VK>();
		pDevice->SetupDevice();

		return pDevice;
	}
}