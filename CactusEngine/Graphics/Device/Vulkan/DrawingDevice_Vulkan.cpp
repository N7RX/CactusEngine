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
	// Alert: we are using directly mapped buffer instead of staging buffer
	// TODO: use staging pool for discrete device and CPU_TO_GPU for integrated device

	RawBufferCreateInfo_Vulkan vertexBufferCreateInfo = {};
	vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	vertexBufferCreateInfo.size = sizeof(float) * createInfo.positionDataCount
		+ sizeof(float) * createInfo.normalDataCount
		+ sizeof(float) * createInfo.texcoordDataCount
		+ sizeof(float) * createInfo.tangentDataCount
		+ sizeof(float) * createInfo.bitangentDataCount;
	vertexBufferCreateInfo.stride = createInfo.interleavedStride * sizeof(float);

	RawBufferCreateInfo_Vulkan indexBufferCreateInfo = {};
	indexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	indexBufferCreateInfo.size = sizeof(int) * createInfo.indexDataCount;
	indexBufferCreateInfo.indexFormat = VK_INDEX_TYPE_UINT32;

	// By default vertex data will be created on discrete device, since integrated device will only handle post processing
	// The alternative is to add a device specifier in VertexBufferCreateInfo
	pOutput = std::make_shared<VertexBuffer_Vulkan>(m_pDevice_0, vertexBufferCreateInfo, indexBufferCreateInfo);

	std::vector<float> interleavedVertices = createInfo.ConvertToInterleavedData();
	void* ppIndexData;
	void* ppVertexData;

	m_pDevice_0->pUploadAllocator->MapMemory(std::static_pointer_cast<VertexBuffer_Vulkan>(pOutput)->GetIndexBufferImpl()->m_allocation, &ppIndexData);
	memcpy(ppIndexData, createInfo.pIndexData, indexBufferCreateInfo.size);
	m_pDevice_0->pUploadAllocator->UnmapMemory(std::static_pointer_cast<VertexBuffer_Vulkan>(pOutput)->GetIndexBufferImpl()->m_allocation);

	m_pDevice_0->pUploadAllocator->MapMemory(std::static_pointer_cast<VertexBuffer_Vulkan>(pOutput)->GetBufferImpl()->m_allocation, &ppVertexData);
	memcpy(ppVertexData, interleavedVertices.data(), vertexBufferCreateInfo.size);
	m_pDevice_0->pUploadAllocator->UnmapMemory(std::static_pointer_cast<VertexBuffer_Vulkan>(pOutput)->GetBufferImpl()->m_allocation);

	return true;
}

bool DrawingDevice_Vulkan::CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput)
{
	Texture2DCreateInfo_Vulkan tex2dCreateInfo = {};
	tex2dCreateInfo.extent = { createInfo.textureWidth, createInfo.textureHeight };
	tex2dCreateInfo.format = VulkanImageFormat(createInfo.format);
	tex2dCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // Alert: TILING_OPTIMAL could be incompatible with certain formats on certain devices
	tex2dCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	tex2dCreateInfo.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	tex2dCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	tex2dCreateInfo.aspect = createInfo.format == ETextureFormat::Depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	tex2dCreateInfo.mipLevels = DetermineMipmapLevels_VK(std::max<uint32_t>(createInfo.textureWidth, createInfo.textureHeight));
	
	auto pDevice = createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1;
	pOutput = std::make_shared<Texture2D_Vulkan>(pDevice, tex2dCreateInfo);

	RawBufferCreateInfo_Vulkan bufferCreateInfo = {};
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
	bufferCreateInfo.size = (VkDeviceSize)createInfo.textureWidth * createInfo.textureHeight * VulkanFormatUnitSize(createInfo.format); // Alert: this size might be incorrect

	auto pStagingBuffer = std::make_shared<RawBuffer_Vulkan>(pDevice, bufferCreateInfo);

	void* ppData;
	pDevice->pUploadAllocator->MapMemory(pStagingBuffer->m_allocation, &ppData);
	memcpy(ppData, createInfo.pTextureData, bufferCreateInfo.size);
	pDevice->pUploadAllocator->UnmapMemory(pStagingBuffer->m_allocation);

	std::vector<VkBufferImageCopy> copyRegions;
	// Only has one level of data
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;  // Tightly packed
	region.bufferImageHeight = 0;// Tightly packed
	region.imageSubresource.aspectMask = createInfo.format == ETextureFormat::Depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { createInfo.textureWidth, createInfo.textureHeight, 1 };
	copyRegions.emplace_back(region);

	// Alert: might have better strategy here
	auto pCmdBuffer = pDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

	pCmdBuffer->CopyBufferToTexture2D(pStagingBuffer, std::static_pointer_cast<Texture2D_Vulkan>(pOutput), copyRegions);
	pCmdBuffer->GenerateMipmap(std::static_pointer_cast<Texture2D_Vulkan>(pOutput));

	pDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);

	return true;
}

bool DrawingDevice_Vulkan::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput)
{
	assert(pOutput == nullptr);

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	pOutput = std::make_shared<FrameBuffer_Vulkan>(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1);
#else
	pOutput = std::make_shared<FrameBuffer_Vulkan>(m_pDevice_0);
#endif
	auto pFrameBuffer = std::static_pointer_cast<FrameBuffer_Vulkan>(pOutput);

	std::vector<VkImageView> viewAttachments;
	for (const auto& pAttachment : createInfo.attachments)
	{
		viewAttachments.emplace_back(std::static_pointer_cast<Texture2D_Vulkan>(pAttachment)->m_imageView);
		pFrameBuffer->m_bufferAttachments.emplace_back(std::static_pointer_cast<Texture2D_Vulkan>(pAttachment));
	}

	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = std::static_pointer_cast<RenderPass_Vulkan>(createInfo.pRenderPass)->m_renderPass;
	frameBufferInfo.attachmentCount = (uint32_t)viewAttachments.size();
	frameBufferInfo.pAttachments = viewAttachments.data();
	frameBufferInfo.width = createInfo.framebufferWidth;
	frameBufferInfo.height = createInfo.framebufferHeight;
	frameBufferInfo.layers = 1;

	return vkCreateFramebuffer(pFrameBuffer->m_pDevice->logicalDevice, &frameBufferInfo, nullptr, &pFrameBuffer->m_frameBuffer) == VK_SUCCESS;
}

void DrawingDevice_Vulkan::ClearRenderTarget()
{

}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments)
{
	std::cerr << "Vulkan: shouldn't call SetRenderTarget on Vulkan device. Call BeginRenderPass instead.\n";
}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
{
	std::cerr << "Vulkan: shouldn't call SetRenderTarget on Vulkan device. Call BeginRenderPass instead.\n";
}

void DrawingDevice_Vulkan::SetClearColor(Color4 color)
{
	m_clearValues.resize(2);
	m_clearValues[0].color = { color.r, color.g, color.b, color.a };
	m_clearValues[1].depthStencil = { 1.0f, 0 };
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
	auto pBuffer = std::static_pointer_cast<VertexBuffer_Vulkan>(pVertexBuffer);

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	auto pCmdBuffer = m_cmdGPUType == EGPUType::Discrete ? m_pDevice_0->pWorkingCmdBuffer : m_pDevice_1->pWorkingCmdBuffer;
#else
	auto pCmdBuffer = m_pDevice_0->pWorkingCmdBuffer;
#endif

	pCmdBuffer->BindVertexBuffer(0, 1, &pBuffer->GetBufferImpl()->m_buffer, 0);
	pCmdBuffer->BindIndexBuffer(pBuffer->GetIndexBufferImpl()->m_buffer, 0, pBuffer->GetIndexFormat());
}

void DrawingDevice_Vulkan::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex)
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	auto pCmdBuffer = m_cmdGPUType == EGPUType::Discrete ? m_pDevice_0->pWorkingCmdBuffer : m_pDevice_1->pWorkingCmdBuffer;
#else
	auto pCmdBuffer = m_pDevice_0->pWorkingCmdBuffer;
#endif
	assert(pCmdBuffer->InRenderPass());

	pCmdBuffer->DrawPrimitiveIndexed(indicesCount, 1, baseIndex, baseVertex);
}

void DrawingDevice_Vulkan::DrawFullScreenQuad()
{

}

void DrawingDevice_Vulkan::ResizeViewPort(uint32_t width, uint32_t height)
{

}

EGraphicsDeviceType DrawingDevice_Vulkan::GetDeviceType() const
{
	return EGraphicsDeviceType::Vulkan;
}

std::shared_ptr<LogicalDevice_Vulkan> DrawingDevice_Vulkan::GetLogicalDevice(EGPUType type) const
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	switch (type)
	{
	case EGPUType::Integrated:
		return m_pDevice_1;
	case EGPUType::Discrete:
		return m_pDevice_0;
	default:
		return nullptr;
	}
#else
	return m_pDevice_0;
#endif
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

bool DrawingDevice_Vulkan::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput)
{
	pOutput = std::make_shared<RenderPass_Vulkan>();

	std::vector<VkAttachmentDescription> attachmentDescs;
	std::vector<VkAttachmentReference> colorAttachmentRefs;
	VkAttachmentReference depthAttachmentRef = {};
	bool duplicateDepthAttachment = false;

	for (int i = 0; i < createInfo.attachmentDescriptions.size(); i++)
	{
		VkAttachmentDescription desc = {};
		desc.format = VulkanImageFormat(createInfo.attachmentDescriptions[i].format);
		desc.samples = VulkanSampleCount(createInfo.attachmentDescriptions[i].sampleCount);
		desc.loadOp = VulkanLoadOp(createInfo.attachmentDescriptions[i].loadOp);
		desc.storeOp = VulkanStoreOp(createInfo.attachmentDescriptions[i].storeOp);
		desc.stencilLoadOp = VulkanLoadOp(createInfo.attachmentDescriptions[i].stencilLoadOp);
		desc.stencilStoreOp = VulkanStoreOp(createInfo.attachmentDescriptions[i].storeOp);
		desc.initialLayout = VulkanImageLayout(createInfo.attachmentDescriptions[i].initialLayout);
		desc.finalLayout = VulkanImageLayout(createInfo.attachmentDescriptions[i].finalLayout);

		attachmentDescs.emplace_back(desc);

		if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Color)
		{
			VkAttachmentReference ref = {};
			ref.attachment = createInfo.attachmentDescriptions[i].index;
			ref.attachment = VulkanImageLayout(createInfo.attachmentDescriptions[i].usageLayout);

			colorAttachmentRefs.emplace_back(ref);
		}
		else if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Depth)
		{
			assert(!duplicateDepthAttachment);
			duplicateDepthAttachment = true;

			depthAttachmentRef.attachment = createInfo.attachmentDescriptions[i].index;
			depthAttachmentRef.layout = VulkanImageLayout(createInfo.attachmentDescriptions[i].usageLayout);
		}
	}

	// TODO: add support for multiple subpasses

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size();
	subpassDesc.pColorAttachments = colorAttachmentRefs.data();
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)attachmentDescs.size();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	return vkCreateRenderPass(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0->logicalDevice : m_pDevice_1->logicalDevice,
		&renderPassInfo, nullptr, &std::static_pointer_cast<RenderPass_Vulkan>(pOutput)->m_renderPass) == VK_SUCCESS;
#else
	return vkCreateRenderPass(m_pDevice_0->logicalDevice,
		&renderPassInfo, nullptr, &std::static_pointer_cast<RenderPass_Vulkan>(pOutput)->m_renderPass) == VK_SUCCESS;
#endif
}

bool DrawingDevice_Vulkan::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput)
{
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;


	return false;
}

void DrawingDevice_Vulkan::SwitchCmdGPUContext(EGPUType type)
{
	m_cmdGPUType = type;
}

void DrawingDevice_Vulkan::BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer)
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	auto pCmdBuffer = m_cmdGPUType == EGPUType::Discrete ? m_pDevice_0->pWorkingCmdBuffer : m_pDevice_1->pWorkingCmdBuffer;
#else
	auto pCmdBuffer = m_pDevice_0->pWorkingCmdBuffer;
#endif
	assert(!pCmdBuffer->InRenderPass());

	// Currently we are only rendering to full window
	pCmdBuffer->BeginRenderPass(std::static_pointer_cast<RenderPass_Vulkan>(pRenderPass)->m_renderPass,
		std::static_pointer_cast<FrameBuffer_Vulkan>(pFrameBuffer)->m_frameBuffer,
		m_clearValues, m_pSwapchain->GetSwapExtent());
}

void DrawingDevice_Vulkan::Present()
{

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