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

		CommandManager_VK*		pGraphicsCommandManager;
		CommandManager_VK*		pTransferCommandManager;
		UploadAllocator_VK*		pUploadAllocator;
		DescriptorAllocator_VK*	pDescriptorAllocator;
		SyncObjectManager_VK*	pSyncObjectManager;

		CommandBuffer_VK*		pImplicitCmdBuffer; // Command buffer used implicitly inside graphics device, for graphics queue
	};

	class GraphicsHardwareInterface_VK : public GraphicsDevice
	{
	public:
		GraphicsHardwareInterface_VK();
		~GraphicsHardwareInterface_VK();

		void Initialize() override;
		void ShutDown() override;

		ShaderProgram* CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, VertexBuffer*& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, Texture2D*& pOutput) override;
		bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, FrameBuffer*& pOutput) override;
		bool CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, UniformBuffer*& pOutput) override;

		void UpdateShaderParameter(ShaderProgram* pShaderProgram, const ShaderParameterTable* pTable, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void SetVertexBuffer(const VertexBuffer* pVertexBuffer, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void DrawFullScreenQuad(GraphicsCommandBuffer* pCommandBuffer) override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		EGraphicsAPIType GetGraphicsAPIType() const override;

		// Low-level functions exclusive to Vulkan device
		void SetupDevice();
		LogicalDevice_VK* GetLogicalDevice() const;

		GraphicsCommandPool* RequestExternalCommandPool(EQueueType queueType) override;
		GraphicsCommandBuffer* RequestCommandBuffer(GraphicsCommandPool* pCommandPool) override;
		void ReturnExternalCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) override;
		GraphicsSemaphore* RequestGraphicsSemaphore(ESemaphoreWaitStage waitStage) override;

		bool CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, DataTransferBuffer*& pOutput) override;
		bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, RenderPassObject*& pOutput) override;
		bool CreateSampler(const TextureSamplerCreateInfo& createInfo, TextureSampler*& pOutput) override;
		bool CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, PipelineVertexInputState*& pOutput) override;
		bool CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, PipelineInputAssemblyState*& pOutput) override;
		bool CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, PipelineColorBlendState*& pOutput) override;
		bool CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, PipelineRasterizationState*& pOutput) override;
		bool CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, PipelineDepthStencilState*& pOutput) override;
		bool CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, PipelineMultisampleState*& pOutput) override;
		bool CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, PipelineViewportState*& pOutput) override;
		bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipelineObject*& pOutput) override;

		void TransitionImageLayout(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void TransitionImageLayout_Immediate(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void ResizeSwapchain(uint32_t width, uint32_t height) override;

		void BindGraphicsPipeline(const GraphicsPipelineObject* pPipeline, GraphicsCommandBuffer* pCommandBuffer) override;
		void BeginRenderPass(const RenderPassObject* pRenderPass, const FrameBuffer* pFrameBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void EndRenderPass(GraphicsCommandBuffer* pCommandBuffer) override;
		void EndCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) override;
		void CommandWaitSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) override;
		void CommandSignalSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) override;

		void Present() override;	
		void FlushCommands(bool waitExecution, bool flushImplicitCommands) override;
		void FlushTransferCommands(bool waitExecution) override;
		void WaitSemaphore(GraphicsSemaphore* pSemaphore) override;

		TextureSampler* GetDefaultTextureSampler(bool withDefaultAF = false) const override;
		void GetSwapchainImages(std::vector<Texture2D*>& outImages) const override;
		uint32_t GetSwapchainPresentImageIndex() const override;

		void CopyTexture2DToDataTransferBuffer(Texture2D* pSrcTexture, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyDataTransferBufferToTexture2D(DataTransferBuffer* pSrcBuffer, Texture2D* pDstTexture, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyDataTransferBuffer(DataTransferBuffer* pSrcBuffer, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyHostDataToDataTransferBuffer(void* pData, DataTransferBuffer* pDstBuffer, size_t size) override;
		void CopyDataTransferBufferToHostDataLocation(DataTransferBuffer* pSrcBuffer, void* pDataLoc) override;

	private:
		// Initialization functions
		void GetRequiredExtensions();
		void CreateInstance();
		void SetupDebugMessenger();
		void CreatePresentationSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void SetupSwapchain();
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void CreateDefaultSampler();

		// Shader-related functions
		void CreateShaderModuleFromFile(const char* shaderFilePath, LogicalDevice_VK* pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode);

		// Manager setup functions
		void SetupCommandManager();
		void SetupSyncObjectManager();
		void SetupUploadAllocator();
		void SetupDescriptorAllocator();

		// Converter functions
		EDescriptorResourceType_VK VulkanDescriptorResourceType(EDescriptorType type) const;
		void GetBufferInfoByDescriptorType(EDescriptorType type, const RawResource* pRes, VkDescriptorBufferInfo& outInfo);

	public:
		const uint64_t FRAME_TIMEOUT = 5e9; // 5 seconds
		const static uint32_t MAX_FRAME_IN_FLIGHT = 2;

	private:
#if defined(DEBUG_MODE_CE)
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
		std::vector<const char*> m_requiredInstanceExtensions;
		std::vector<const char*> m_requiredDeviceExtensions;
		std::vector<const char*> m_validationLayers;
		std::vector<VkExtensionProperties> m_availableExtensions;

		LogicalDevice_VK* m_pMainDevice;

		Swapchain_VK* m_pSwapchain;
		uint32_t m_currentFrame;

		Sampler_VK* m_pDefaultSampler_0; // No AF
		Sampler_VK* m_pDefaultSampler_1; // 4x AF
		//... (Extend samplers)
	};

	template<>
	static GraphicsDevice* CreateGraphicsDevice<EGraphicsAPIType::Vulkan>()
	{
		if (!gpGlobal->QueryGlobalState(EGlobalStateQueryType::VulkanInit))
		{
			// Attempt to load Vulkan loader from the system
			if (volkInitialize() != VK_SUCCESS)
			{
				throw std::runtime_error("Vulkan: Failed to initialize Volk.");
				return nullptr;
			}
			else
			{
				gpGlobal->MarkGlobalState(EGlobalStateQueryType::VulkanInit, true);
			}
		}

		GraphicsHardwareInterface_VK* pDevice = nullptr;
		CE_NEW(pDevice, GraphicsHardwareInterface_VK);
		pDevice->SetupDevice();

		return pDevice;
	}
}