#pragma once
#include "DrawingDevice.h"
#include "DrawingExtensionWrangler_Vulkan.h"
#include "DrawingCommandManager_Vulkan.h"
#include "DrawingResources_Vulkan.h"

namespace Engine
{
	enum PhysicalDeviceType_Vulkan
	{
		eVulkanPhysicalDeviceType_Integrated = 0,
		eVulkanPhysicalDeviceType_Discrete
	};

	struct LogicalDevice_Vulkan
	{
		PhysicalDeviceType_Vulkan type;
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
		VkPhysicalDeviceProperties deviceProperties;
		DrawingCommandQueue_Vulkan presentQueue;
		DrawingCommandQueue_Vulkan graphicsQueue;
		std::shared_ptr<DrawingCommandManager_Vulkan> pGraphicsCommandManager;
#if defined(ENABLE_COPY_QUEUE_VK)
		DrawingCommandQueue_Vulkan copyQueue;
		std::shared_ptr<DrawingCommandManager_Vulkan> pCopyCommandManager;
#endif
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

		void SetupDevice();
		void Initialize() override;
		void ShutDown() override;

		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;

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

		void Present() override;

		EGraphicsDeviceType GetDeviceType() const override;

		VkPhysicalDevice GetPhysicalDevice(PhysicalDeviceType_Vulkan type) const;
		VkDevice GetLogicalDevice(PhysicalDeviceType_Vulkan type) const;

		void ConfigureStates_Test() override;

	private:
		void GetRequiredExtensions();
		void CreateInstance();
		void SetupDebugMessenger();
		void CreatePresentationSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateLogicalDevice(std::shared_ptr<LogicalDevice_Vulkan> pDevice);

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

		std::shared_ptr<LogicalDevice_Vulkan> m_pDiscreteDevice;
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
		std::shared_ptr<LogicalDevice_Vulkan> m_pIntegratedDevice;
#endif

		VkApplicationInfo m_appInfo;
		std::vector<const char*> m_requiredExtensions;
		std::vector<VkExtensionProperties> m_availableExtensions;

		VkSurfaceKHR m_presentationSurface;

		VkDebugUtilsMessengerEXT m_debugMessenger;
	};

	template<>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice<eDevice_Vulkan>()
	{
		auto pDevice = std::make_shared<DrawingDevice_Vulkan>();
		pDevice->SetupDevice();

		return pDevice;
	}
}