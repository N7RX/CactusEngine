#pragma once
#include "DrawingDevice.h"
#include "DrawingExtensionWrangler_Vulkan.h"
#include "DrawingCommandManager_Vulkan.h"
#include "DrawingResources_Vulkan.h"

namespace Engine
{
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
		void SetupDevice();
		void Initialize() override;
		void ShutDown() override;

		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) override;

		void SetClearColor(Color4 color) override;
		void ClearTarget() override;
		void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable) override;
		void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex = 0, uint32_t baseVertex = 0) override;

		void Present() override;

		EGraphicsDeviceType GetDeviceType() const override;

		VkPhysicalDevice GetPhysicalDevice() const;
		VkDevice GetLogicalDevice() const;

		void ConfigureStates_Test() override;

	private:
		void GetRequiredExtensions();
		void CreateInstance();
		void SetupDebugMessenger();
		void CreatePresentationSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
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
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_logicalDevice;

		VkApplicationInfo m_appInfo;
		std::vector<const char*> m_requiredExtensions;
		VkPhysicalDeviceProperties m_deviceProperties;
		std::vector<VkExtensionProperties> m_availableExtensions;

		VkSurfaceKHR m_presentationSurface;

		VkDebugUtilsMessengerEXT m_debugMessenger;

		DrawingCommandQueue_Vulkan m_presentQueue;
		DrawingCommandQueue_Vulkan m_graphicsQueue;
		std::shared_ptr<DrawingCommandManager_Vulkan> m_pGraphicsCommandManager;
#if defined(ENABLE_COPY_QUEUE_VK)
		DrawingCommandQueue_Vulkan m_copyQueue;
		std::shared_ptr<DrawingCommandManager_Vulkan> m_pCopyCommandManager;		
#endif
	};

	template<>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice<eDevice_Vulkan>()
	{
		auto pDevice = std::make_shared<DrawingDevice_Vulkan>();
		pDevice->SetupDevice();

		return pDevice;
	}
}