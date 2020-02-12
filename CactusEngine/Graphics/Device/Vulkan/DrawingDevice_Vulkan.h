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
		EGPUType type;
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

		std::shared_ptr<DrawingCommandBuffer_Vulkan> pWorkingCmdBuffer;
	};

	class DrawingDevice_Vulkan : public DrawingDevice
	{
	public:
		const VkPresentModeKHR m_presentModes[4] =
		{
			// From first choice to fallback
			VK_PRESENT_MODE_IMMEDIATE_KHR,
			VK_PRESENT_MODE_MAILBOX_KHR,
			VK_PRESENT_MODE_FIFO_KHR,
			VK_PRESENT_MODE_FIFO_RELAXED_KHR
		};

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

		void ClearRenderTarget() override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments) override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer) override;
		void SetClearColor(Color4 color) override;
		void SetBlendState(const DeviceBlendStateInfo& blendInfo) override;
		void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable) override;
		void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex) override;
		void DrawFullScreenQuad() override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		EGraphicsDeviceType GetDeviceType() const override;

		// Low-level functions exclusive to Vulkan device
		void SetupDevice();
		std::shared_ptr<LogicalDevice_Vulkan> GetLogicalDevice(EGPUType type) const;

		bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput) override;
		bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput) override;

		void SwitchCmdGPUContext(EGPUType type) override;
		void BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer) override;
		void Present() override;		

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
		
		// Shader-related functions
		void CreateShaderModuleFromFile(const char* shaderFilePath, std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode);
		
		// Pipeline state functions
		void CreatePipelineVertexInputState(std::shared_ptr<PipelineVertexInputState_Vulkan> pOutInputState);
		void CreatePipelineInputAssemblyState(std::shared_ptr<VkPipelineInputAssemblyStateCreateInfo> pOutAssemblyState);
		void CreatePipelineColorBlendState(std::shared_ptr<PipelineColorBlendState_Vulkan> pOutBlendState);
		void CreatePipelineRasterizationState(std::shared_ptr<VkPipelineRasterizationStateCreateInfo> pOutRasterizationState);
		void CreatePipelineDepthStencilState(std::shared_ptr<VkPipelineDepthStencilStateCreateInfo> pOutDepthStencilState);
		void CreatePipelineMultisampleState(std::shared_ptr<VkPipelineMultisampleStateCreateInfo> pOutMultisampleState);
		void CreatePipelineViewportState(std::shared_ptr<PipelineViewportState_Vulkan> pOutViewportState);

		// Manager setup functions
		void SetupCommandManager();
		void SetupUploadAllocator();
		void SetupDescriptorAllocator();

	private:
#if defined(_DEBUG)
		const bool m_enableValidationLayers = true;
#else
		const bool m_enableValidationLayers = false;
#endif
		// Shader locations
		const uint32_t ATTRIB_POSITION_LOCATION = 0;
		const uint32_t ATTRIB_NORMAL_LOCATION = 1;
		const uint32_t ATTRIB_TEXCOORD_LOCATION = 2;

	private:
		VkInstance m_instance;

		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice_0; // Discrete GPU
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
		std::shared_ptr<LogicalDevice_Vulkan> m_pDevice_1; // Integrated GPU
#endif		

		VkApplicationInfo m_appInfo;
		std::vector<const char*> m_requiredExtensions;
		std::vector<VkExtensionProperties> m_availableExtensions;

		EGPUType m_cmdGPUType;
		VkSurfaceKHR m_presentationSurface;
		std::shared_ptr<DrawingSwapchain_Vulkan> m_pSwapchain;

		VkDebugUtilsMessengerEXT m_debugMessenger;
		std::vector<VkClearValue> m_clearValues; // Color + Depth stencil
	};

	template<>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice<EGraphicsDeviceType::Vulkan>()
	{
		auto pDevice = std::make_shared<DrawingDevice_Vulkan>();
		pDevice->SetupDevice();

		return pDevice;
	}
}