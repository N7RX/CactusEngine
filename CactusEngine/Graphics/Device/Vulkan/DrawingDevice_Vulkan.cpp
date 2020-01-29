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
	SetupCommandManager();
	SetupUploadAllocator();
	SetupDescriptorAllocator();
}

void DrawingDevice_Vulkan::ShutDown()
{

}

std::shared_ptr<ShaderProgram> DrawingDevice_Vulkan::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	std::cerr << "Vulkan: must specify GPU type when creating shader in heterogeneous-GPU rendering mode.\n";
	return nullptr;
#else
	VkShaderModule vertexModule = VK_NULL_HANDLE;
	VkShaderModule fragmentModule = VK_NULL_HANDLE;

	std::vector<char> vertexRawCode;
	std::vector<char> fragmentRawCode;

	CreateShaderModuleFromFile(vertexShaderFilePath, m_pDevice_0, vertexModule, vertexRawCode);
	CreateShaderModuleFromFile(fragmentShaderFilePath, m_pDevice_0, fragmentModule, fragmentRawCode);

	auto pVertexShader = std::make_shared<VertexShader_Vulkan>(m_pDevice_0, vertexModule, vertexRawCode);
	auto pFragmentShader = std::make_shared<FragmentShader_Vulkan>(m_pDevice_0, fragmentModule, fragmentRawCode);

	auto pShaderProgram = std::make_shared<ShaderProgram_Vulkan>(this, m_pDevice_0, 2, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());

	return pShaderProgram;
#endif
}

std::shared_ptr<ShaderProgram> DrawingDevice_Vulkan::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType)
{
	VkShaderModule vertexModule = VK_NULL_HANDLE;
	VkShaderModule fragmentModule = VK_NULL_HANDLE;

	std::vector<char> vertexRawCode;
	std::vector<char> fragmentRawCode;

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	switch (gpuType)
	{
	case EGPUType::Discrete:
	{
		CreateShaderModuleFromFile(vertexShaderFilePath, m_pDevice_0, vertexModule, vertexRawCode);
		CreateShaderModuleFromFile(fragmentShaderFilePath, m_pDevice_0, fragmentModule, fragmentRawCode);

		auto pVertexShader = std::make_shared<VertexShader_Vulkan>(m_pDevice_0, vertexModule, vertexRawCode);
		auto pFragmentShader = std::make_shared<FragmentShader_Vulkan>(m_pDevice_0, fragmentModule, fragmentRawCode);

		auto pShaderProgram = std::make_shared<ShaderProgram_Vulkan>(this, m_pDevice_0, 2, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());

		return pShaderProgram;
	}
	case EGPUType::Integrated:
	{
		CreateShaderModuleFromFile(vertexShaderFilePath, m_pDevice_1, vertexModule, vertexRawCode);
		CreateShaderModuleFromFile(fragmentShaderFilePath, m_pDevice_1, fragmentModule, fragmentRawCode);
		
		auto pVertexShader = std::make_shared<VertexShader_Vulkan>(m_pDevice_1, vertexModule, vertexRawCode);
		auto pFragmentShader = std::make_shared<FragmentShader_Vulkan>(m_pDevice_1, fragmentModule, fragmentRawCode);

		auto pShaderProgram = std::make_shared<ShaderProgram_Vulkan>(this, m_pDevice_1, 2, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());

		return pShaderProgram;
	}
	default:
		std::cerr << "Vulkan: unhandled GPU type.\n";
		return nullptr;
	}
#else
	// GPU type specification will be ignored
	CreateShaderModuleFromFile(vertexShaderFilePath, m_pDevice_0, vertexModule, vertexRawCode);
	CreateShaderModuleFromFile(fragmentShaderFilePath, m_pDevice_0, fragmentModule, fragmentRawCode);

	auto pVertexShader = std::make_shared<VertexShader_Vulkan>(m_pDevice_0, vertexModule, vertexRawCode);
	auto pFragmentShader = std::make_shared<FragmentShader_Vulkan>(m_pDevice_0, fragmentModule, fragmentRawCode);

	auto pShaderProgram = std::make_shared<ShaderProgram_Vulkan>(this, m_pDevice_0, 2, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());

	return pShaderProgram;
#endif
}

bool DrawingDevice_Vulkan::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput)
{
	RawBufferCreateInfo_Vulkan vertexBufferCreateInfo = {};
	vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	vertexBufferCreateInfo.size = sizeof(float) * createInfo.positionDataCount
		+ sizeof(float) * createInfo.normalDataCount
		+ sizeof(float) * createInfo.texcoordDataCount
		+ sizeof(float) * createInfo.tangentDataCount
		+ sizeof(float) * createInfo.bitangentDataCount;
	vertexBufferCreateInfo.stride = (3 + 3 + 2 + 3 + 3) * sizeof(float);

	RawBufferCreateInfo_Vulkan indexBufferCreateInfo = {};
	indexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	indexBufferCreateInfo.size = sizeof(int) * createInfo.indexDataCount;
	indexBufferCreateInfo.indexFormat = VK_INDEX_TYPE_UINT32;

	// Need to re-combine [position, normal, texcoord, tangent, bitangent] into per vertex data
	

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

bool DrawingDevice_Vulkan::CreateImageView(const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const VkImageViewCreateInfo& createInfo, VkImageView& outImageView)
{
	assert(pLogicalDevice);

	if (vkCreateImageView(pLogicalDevice->logicalDevice, &createInfo, nullptr, &outImageView) == VK_SUCCESS)
	{
		return true;
	}

	outImageView = VK_NULL_HANDLE;
	return false;
}

void DrawingDevice_Vulkan::ClearRenderTarget()
{

}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments)
{

}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
{

}

void DrawingDevice_Vulkan::SetClearColor(Color4 color)
{

}

void DrawingDevice_Vulkan::SetBlendState(const DeviceBlendStateInfo& blendInfo)
{

}

void DrawingDevice_Vulkan::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable)
{
	auto pProgram = std::static_pointer_cast<ShaderProgram_Vulkan>(pShaderProgram);


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

void DrawingDevice_Vulkan::ResizeViewPort(uint32_t width, uint32_t height)
{

}

void DrawingDevice_Vulkan::Present()
{

}

EGraphicsDeviceType DrawingDevice_Vulkan::GetDeviceType() const
{
	return EGraphicsDeviceType::Vulkan;
}

VkPhysicalDevice DrawingDevice_Vulkan::GetPhysicalDevice(PhysicalDeviceType_Vulkan type) const
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	switch (type)
	{
	case PhysicalDeviceType_Vulkan::Integrated:
		return m_pDevice_1->physicalDevice;
	case PhysicalDeviceType_Vulkan::Discrete:
		return m_pDevice_0->physicalDevice;
	default:
		return VK_NULL_HANDLE;
	}
#else
	return m_pDevice_0->physicalDevice;
#endif
}

VkDevice DrawingDevice_Vulkan::GetLogicalDevice(PhysicalDeviceType_Vulkan type) const
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	switch (type)
	{
	case PhysicalDeviceType_Vulkan::Integrated:
		return m_pDevice_1->logicalDevice;
	case PhysicalDeviceType_Vulkan::Discrete:
		return m_pDevice_0->logicalDevice;
	default:
		return VK_NULL_HANDLE;
	}
#else
	return m_pDevice_0->logicalDevice;
#endif
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
	m_appInfo.pApplicationName = gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->GetAppName();
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

	std::vector<VkPhysicalDevice> suitableDevices;
	for (const auto& device : devices)
	{
		if (IsPhysicalDeviceSuitable_VK(device, m_presentationSurface, m_deviceExtensions))
		{
			suitableDevices.emplace_back(device);
		}
	}

	if (suitableDevices.size() == 0)
	{
		throw std::runtime_error("Vulkan: Failed to find a suitable GPU.");
		return;
	}

	m_pDevice_0 = std::make_shared< LogicalDevice_Vulkan>();
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	m_pDevice_1 = std::make_shared< LogicalDevice_Vulkan>();
#endif

	for (const auto& device : suitableDevices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			m_pDevice_0->physicalDevice = device;
			m_pDevice_0->deviceProperties = deviceProperties;

#if defined(_DEBUG)
			PrintPhysicalDeviceInfo_VK(deviceProperties);
#endif
		}
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
		else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			m_pDevice_1->physicalDevice = device;
			m_pDevice_1->deviceProperties = deviceProperties;

#if defined(_DEBUG)
			PrintPhysicalDeviceInfo_VK(deviceProperties);
#endif
		}
#endif
	}
}

void DrawingDevice_Vulkan::CreateLogicalDevice()
{
	CreateLogicalDevice(m_pDevice_0);
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	CreateLogicalDevice(m_pDevice_1);
#endif
}

void DrawingDevice_Vulkan::CreateLogicalDevice(std::shared_ptr<LogicalDevice_Vulkan> pDevice)
{
	QueueFamilyIndices_VK queueFamilyIndices = FindQueueFamilies_VK(pDevice->physicalDevice, m_presentationSurface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies;

	if (queueFamilyIndices.graphicsFamily.has_value())
	{
		uniqueQueueFamilies.emplace(queueFamilyIndices.graphicsFamily.value());
	}
	if (queueFamilyIndices.presentFamily.has_value())
	{
		uniqueQueueFamilies.emplace(queueFamilyIndices.presentFamily.value());
	}
#if defined(ENABLE_COPY_QUEUE_VK)
	if (queueFamilyIndices.copyFamily.has_value())
	{
		uniqueQueueFamilies.emplace(queueFamilyIndices.copyFamily.value());
	}
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

	VkResult result = vkCreateDevice(pDevice->physicalDevice, &deviceCreateInfo, nullptr, &pDevice->logicalDevice);

	if (result != VK_SUCCESS || pDevice->logicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan: Failed to create logical device.");
		return;
	}

	if (queueFamilyIndices.graphicsFamily.has_value())
	{
		pDevice->graphicsQueue.type = EQueueType::Graphics;
		pDevice->graphicsQueue.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		vkGetDeviceQueue(pDevice->logicalDevice, pDevice->graphicsQueue.queueFamilyIndex, 0, &pDevice->graphicsQueue.queue);
	}

	if (queueFamilyIndices.presentFamily.has_value())
	{
		pDevice->presentQueue.type = EQueueType::Present;
		pDevice->presentQueue.queueFamilyIndex = queueFamilyIndices.presentFamily.value();
		vkGetDeviceQueue(pDevice->logicalDevice, pDevice->presentQueue.queueFamilyIndex, 0, &pDevice->presentQueue.queue);
	}

#if defined(ENABLE_COPY_QUEUE_VK)
	if (queueFamilyIndices.copyFamily.has_value())
	{
		pDevice->copyQueue.type = EQueueType::Copy;
		pDevice->copyQueue.queueFamilyIndex = queueFamilyIndices.copyFamily.value();
		vkGetDeviceQueue(pDevice->logicalDevice, pDevice->copyQueue.queueFamilyIndex, 0, &pDevice->copyQueue.queue);
	}
#endif

#if defined(_DEBUG)
	std::cout << "Vulkan: Logical device created on " << pDevice->deviceProperties.deviceName << "\n";
#endif
}

void DrawingDevice_Vulkan::CreateShaderModuleFromFile(const char* shaderFilePath, std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode)
{
	std::ifstream file(shaderFilePath, std::ios::ate | std::ios::binary | std::ios::in);

	if (!file.is_open())
	{
		throw std::runtime_error("Vulkan: failed to open " + *shaderFilePath);
		return;
	}

	size_t fileSize = (size_t)file.tellg();
	assert(fileSize > 0);
	outRawCode.resize(fileSize);

	file.seekg(0, std::ios::beg);
	file.read(outRawCode.data(), fileSize);

	file.close();
	assert(outRawCode.size() > 0);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = reinterpret_cast<uint32_t*>(outRawCode.data());

	vkCreateShaderModule(pLogicalDevice->logicalDevice, &createInfo, nullptr, &outModule);
	assert(outModule != VK_NULL_HANDLE);
}

void DrawingDevice_Vulkan::SetupCommandManager()
{
	m_pDevice_0->pGraphicsCommandManager = std::make_shared<DrawingCommandManager_Vulkan>(m_pDevice_0, m_pDevice_0->graphicsQueue);
#if defined(ENABLE_COPY_QUEUE_VK)
	m_pDevice_0->pCopyCommandManager = std::make_shared<DrawingCommandManager_Vulkan>(m_pDevice_0, m_pDevice_0->copyQueue);
#endif

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	m_pDevice_1->pGraphicsCommandManager = std::make_shared<DrawingCommandManager_Vulkan>(m_pDevice_1, m_pDevice_1->graphicsQueue);
#if defined(ENABLE_COPY_QUEUE_VK)
	// UHD 630 GPU doesn't seem to have standalone copy queue
#endif
#endif
}

void DrawingDevice_Vulkan::SetupUploadAllocator()
{
	m_pDevice_0->pUploadAllocator = std::make_shared<DrawingUploadAllocator_Vulkan>(m_pDevice_0);
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	m_pDevice_1->pUploadAllocator = std::make_shared<DrawingUploadAllocator_Vulkan>(m_pDevice_1);
#endif
}

void DrawingDevice_Vulkan::SetupDescriptorAllocator()
{
	m_pDevice_0->pDescriptorAllocator = std::make_shared<DrawingDescriptorAllocator_Vulkan>(m_pDevice_0);
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	m_pDevice_1->pDescriptorAllocator = std::make_shared<DrawingDescriptorAllocator_Vulkan>(m_pDevice_1);
#endif

	system("pause");
}