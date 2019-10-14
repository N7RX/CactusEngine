#include "Global.h"
#include "DrawingDevice_Vulkan.h"
#include "DrawingResources_Vulkan.h"
#include "DrawingUtil_Vulkan.h"
#include <set>
#if defined(GLFW_IMPLEMENTATION_CACTUS)
#include <GLFW/glfw3.h>
#endif

using namespace Engine;

DrawingDevice_Vulkan::~DrawingDevice_Vulkan()
{
	ShutDown();
}

void DrawingDevice_Vulkan::SetupDevice()
{
	GetRequiredExtensions();
	CreateInstance();
	SetupDebugMessenger();
	CreatePresentationSurface();
	SelectPhysicalDevice();
	CreateLogicalDevice();
}

void DrawingDevice_Vulkan::Initialize()
{
	
}

void DrawingDevice_Vulkan::ShutDown()
{

}

std::shared_ptr<ShaderProgram> DrawingDevice_Vulkan::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
{
	return nullptr;
}

bool DrawingDevice_Vulkan::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput)
{
	return false;
}

bool DrawingDevice_Vulkan::CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput)
{
	return false;
}

bool DrawingDevice_Vulkan::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput)
{
	return false;
}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
{

}

void DrawingDevice_Vulkan::SetClearColor(Color4 color)
{

}

void DrawingDevice_Vulkan::ClearTarget()
{

}

void DrawingDevice_Vulkan::SetBlendState(const DeviceBlendStateInfo& blendInfo)
{

}

void DrawingDevice_Vulkan::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable)
{

}

void DrawingDevice_Vulkan::SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer)
{

}

void DrawingDevice_Vulkan::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex)
{

}

void DrawingDevice_Vulkan::DrawFullScreenQuad()
{

}

void DrawingDevice_Vulkan::Present()
{

}

EGraphicsDeviceType DrawingDevice_Vulkan::GetDeviceType() const
{
	return eDevice_Vulkan;
}

VkPhysicalDevice DrawingDevice_Vulkan::GetPhysicalDevice() const
{
	return m_physicalDevice;
}

VkDevice DrawingDevice_Vulkan::GetLogicalDevice() const
{
	return m_logicalDevice;
}

void DrawingDevice_Vulkan::ConfigureStates_Test()
{

}

void DrawingDevice_Vulkan::GetRequiredExtensions()
{
	m_requiredExtensions = GetRequiredExtensions_VK(m_enableValidationLayers);
}

void DrawingDevice_Vulkan::CreateInstance()
{
	// Check validation layers
	if (m_enableValidationLayers && !CheckValidationLayerSupport_VK(m_validationLayers))
	{
		throw std::runtime_error("Validation layers requested, but not available.");
	}

	// Check extentions support
	if (!CheckAvailableInstanceExtensions_VK(m_availableExtensions))
	{
		return;
	}
	for (auto& extension : m_requiredExtensions)
	{
		if (!IsExtensionSupported_VK(m_availableExtensions, extension))
		{
			std::cout << "Vulkan: Extension '" << extension << "' is not supported." << std::endl;
			return;
		}
	}

	// Generate application info
	m_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	m_appInfo.pApplicationName = gpGlobal->GetConfiguration<AppConfiguration>(eConfiguration_App)->GetAppName();
	m_appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	m_appInfo.pEngineName = "Cactus Engine";
	m_appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	m_appInfo.apiVersion = VK_API_VERSION_1_1;

	// Generate creation info
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &m_appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = m_requiredExtensions.size() > 0 ? m_requiredExtensions.data() : nullptr;

	// Configure debug messenger
	if (m_enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)& GetDebugUtilsMessengerCreateInfo_VK();
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.pNext = nullptr;
	}

	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
	if ((result != VK_SUCCESS) || m_instance == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Could not create Vulkan instance.");
	}
}

void DrawingDevice_Vulkan::SetupDebugMessenger()
{
	if (!m_enableValidationLayers)
	{
		return;
	}

	VkResult result = CreateDebugUtilsMessengerEXT_VK(m_instance, nullptr, &m_debugMessenger);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Vulkan: Failed to setup debug messenger");
	}
}

void DrawingDevice_Vulkan::CreatePresentationSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)

#if defined(GLFW_IMPLEMENTATION_CACTUS)
	auto glfwWindow = reinterpret_cast<GLFWwindow*>(gpGlobal->GetWindowHandle());
	VkResult result = glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &m_presentationSurface);
#else
	VkWin32SurfaceCreateInfoKHR surfaceCreationInfo;
	surfaceCreationInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreationInfo.hinstance = GetModuleHandle(nullptr);
	surfaceCreationInfo.hwnd = reinterpret_cast<HWND>(gpGlobal->GetWindowHandle());
	surfaceCreationInfo.pNext = nullptr;
	surfaceCreationInfo.flags = 0;

	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreationInfo, nullptr, &m_presentationSurface);
#endif

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	throw std::runtime_error("Vulkan: PLATFORM_XLIB is not supported.");

#elif defined(VK_USE_PLATFORM_XCB_KHR)
	throw std::runtime_error("Vulkan: PLATFORM_XCB is not supported.");

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	throw std::runtime_error("Vulkan: PLATFORM_ANDROID is not supported.");

#elif defined(VK_USE_PLATFORM_IOS_MVK)
	throw std::runtime_error("Vulkan: PLATFORM_IOS is not supported.");

#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	throw std::runtime_error("Vulkan: PLATFORM_MACOS is not supported.");

#endif

	if ((result != VK_SUCCESS) || m_presentationSurface == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan: Could not create presentation surface.");
	}
}

void DrawingDevice_Vulkan::SelectPhysicalDevice()
{
	uint32_t deviceCount = 0;
	VkResult result;

	result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0 || (result != VK_SUCCESS))
	{
		throw std::runtime_error("Failed to find GPU with Vulkan support.");
		return;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (IsPhysicalDeviceSuitable_VK(device, m_presentationSurface, m_deviceExtensions))
		{
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan: Failed to find a suitable GPU.");
		return;
	}

	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
#if defined(_DEBUG)
	PrintPhysicalDeviceInfo_VK(m_deviceProperties);
#endif
}

void DrawingDevice_Vulkan::CreateLogicalDevice()
{
	QueueFamilyIndices_VK queueFamilyIndices = FindQueueFamilies_VK(m_physicalDevice, m_presentationSurface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
#if defined(ENABLE_COPY_QUEUE_VK)
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(), queueFamilyIndices.copyFamily.value() };
#else
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
#endif

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &m_deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (m_enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice);

	if (result != VK_SUCCESS || m_logicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan: Failed to create logical device.");
		return;
	}

	m_graphicsQueue.type = eQueue_Graphics;
	m_graphicsQueue.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	vkGetDeviceQueue(m_logicalDevice, m_graphicsQueue.queueFamilyIndex, 0, &m_graphicsQueue.queue);

	m_presentQueue.type = eQueue_Present;
	m_presentQueue.queueFamilyIndex = queueFamilyIndices.presentFamily.value();	
	vkGetDeviceQueue(m_logicalDevice, m_presentQueue.queueFamilyIndex, 0, &m_presentQueue.queue);
#if defined(ENABLE_COPY_QUEUE_VK)
	m_copyQueue.type = eQueue_Copy;
	m_copyQueue.queueFamilyIndex = queueFamilyIndices.copyFamily.value();
	vkGetDeviceQueue(m_logicalDevice, m_copyQueue.queueFamilyIndex, 0, &m_copyQueue.queue);
#endif
}

void DrawingDevice_Vulkan::SetupCommandManager()
{

}

void DrawingDevice_Vulkan::SetupUploadAllocator()
{

}

void DrawingDevice_Vulkan::SetupDescriptorAllocator()
{

}