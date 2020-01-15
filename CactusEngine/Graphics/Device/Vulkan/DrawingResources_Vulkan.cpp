#include "DrawingResources_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include "BuiltInShaderType.h"

#include <cstdarg>

using namespace Engine;

Texture2D_Vulkan::Texture2D_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const Texture2DCreateInfo_Vulkan& createInfo)
	: m_pDrawingDevice(pDrawingDevice), m_pLogicalDevice(pLogicalDevice), m_allocatorType(EAllocatorType_Vulkan::VMA)
{
	assert(pLogicalDevice);
	assert(pDrawingDevice);

	pLogicalDevice->pUploadAllocator->CreateTexture2D(createInfo, *this);

	m_format = createInfo.format;
	m_extent = createInfo.extent;
	m_mipLevels = createInfo.mipLevels;

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_image;
	viewCreateInfo.viewType = createInfo.viewType;
	viewCreateInfo.format = createInfo.format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.subresourceRange.aspectMask = createInfo.aspect;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = createInfo.mipLevels;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	m_pDrawingDevice->CreateImageView(pLogicalDevice, viewCreateInfo, m_imageView);
}

Texture2D_Vulkan::~Texture2D_Vulkan()
{
	if (m_allocatorType == EAllocatorType_Vulkan::VMA)
	{
		m_pLogicalDevice->pUploadAllocator->FreeImage(m_image, m_allocation);
	}

	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pLogicalDevice->logicalDevice, m_imageView, nullptr);
	}
}

uint32_t Texture2D_Vulkan::GetTextureID() const
{
	return -1; // This is currently not used in Vulkan device
}

RenderTarget2D_Vulkan::RenderTarget2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat)
{
	m_pLogicalDevice = pLogicalDevice;

	m_image = targetImage;
	m_imageView = targetView;
	m_extent = targetExtent;
	m_format = targetFormat;

	m_allocatorType = EAllocatorType_Vulkan::VK;
}

RenderTarget2D_Vulkan::~RenderTarget2D_Vulkan()
{
	if (m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_pLogicalDevice->logicalDevice, m_image, nullptr);
	}
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pLogicalDevice->logicalDevice, m_imageView, nullptr);
	}
}

RawBuffer_Vulkan::RawBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& createInfo)
	: m_pDevice(pDevice), m_deviceSize(createInfo.size)
{
	assert(pDevice);

	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
	m_sizeInBytes = (uint32_t)createInfo.size;

	if (m_deviceSize > 0)
	{
		m_pDevice->pUploadAllocator->CreateBuffer(createInfo, *this);
	}
}

RawBuffer_Vulkan::~RawBuffer_Vulkan()
{
	if (m_buffer != VK_NULL_HANDLE)
	{
		m_pDevice->pUploadAllocator->FreeBuffer(m_buffer, m_allocation);
	}
}

UniformBuffer_Vulkan::UniformBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const UniformBufferCreateInfo_Vulkan& createInfo)
	: RawBuffer_Vulkan(pDevice, { VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, createInfo.size, 0 }), m_eType(createInfo.type), 
	m_layoutParamIndex(createInfo.layoutParamIndex), m_appliedShaderStage(createInfo.appliedStages), m_memoryMapped(false), m_pHostData(nullptr)
{
	m_sizeInBytes = createInfo.size;
}

UniformBuffer_Vulkan::~UniformBuffer_Vulkan()
{
	if (m_memoryMapped)
	{
		m_pDevice->pUploadAllocator->UnmapMemory(m_allocation);
	}
}

void UniformBuffer_Vulkan::UpdateBufferData(const std::shared_ptr<void> pData)
{
	m_pRawData->m_pData = pData;
}

void UniformBuffer_Vulkan::UpdateToDevice(std::shared_ptr<DrawingCommandBuffer_Vulkan> pCmdBuffer)
{
	assert(m_pDevice);
	assert(m_pRawData);

	switch (m_eType)
	{
	case EUniformBufferType_Vulkan::PushConstant:
		assert(pCmdBuffer);
		pCmdBuffer->UpdatePushConstant(m_appliedShaderStage, m_sizeInBytes, m_pRawData->m_pData.get());
		break;

	case EUniformBufferType_Vulkan::Uniform:
		if (!m_memoryMapped)
		{
			m_memoryMapped = m_pDevice->pUploadAllocator->MapMemory(m_allocation, &m_pHostData);
		}
		if (m_memoryMapped)
		{
			memcpy(m_pHostData, m_pRawData->m_pData.get(), m_sizeInBytes);
		}
		else
		{
			throw std::runtime_error("Vulkan: failed to map uniform buffer memory.");
			return;
		}
		break;

	default:
		throw std::runtime_error("Vulkan: unhandled uniform buffer type.");
		return;
	}
}

EUniformBufferType_Vulkan UniformBuffer_Vulkan::GetType() const
{
	return m_eType;
}

uint32_t UniformBuffer_Vulkan::GetBindingIndex() const
{
	return m_layoutParamIndex;
}

FrameBuffer_Vulkan::~FrameBuffer_Vulkan()
{
	vkDestroyFramebuffer(m_pDevice->logicalDevice, m_frameBuffer, nullptr);
}

uint32_t FrameBuffer_Vulkan::GetFrameBufferID() const
{
	return -1; // Vulkan device does not make use of this ID
}

DrawingSwapchain_Vulkan::DrawingSwapchain_Vulkan(const std::shared_ptr<DrawingDevice_Vulkan> pDrawingDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, const DrawingSwapchainCreateInfo_Vulkan& createInfo)
	: m_pLogicalDevice(pLogicalDevice), m_presentQueue(VK_NULL_HANDLE), m_frameBufferIndex(0)
{
	uint32_t imageCount = createInfo.supportDetails.capabilities.minImageCount - 1 + createInfo.maxFramesInFlight;
	if (createInfo.supportDetails.capabilities.maxImageCount > 0 && imageCount > createInfo.supportDetails.capabilities.maxImageCount)
	{
		imageCount = createInfo.supportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfoKHR;
	createInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfoKHR.surface = createInfo.surface;
	createInfoKHR.minImageCount = imageCount;
	createInfoKHR.imageFormat = createInfo.surfaceFormat.format;
	createInfoKHR.imageColorSpace = createInfo.surfaceFormat.colorSpace;
	createInfoKHR.imageExtent = createInfo.swapExtent;
	createInfoKHR.imageArrayLayers = 1;
	createInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfoKHR.preTransform = createInfo.supportDetails.capabilities.currentTransform;
	createInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfoKHR.presentMode = createInfo.presentMode;
	createInfoKHR.clipped = VK_TRUE;

	uint32_t queueFamilyIndices[] = { createInfo.queueFamilyIndices.graphicsFamily.value(), createInfo.queueFamilyIndices.presentFamily.value() };

	if (queueFamilyIndices[0] != queueFamilyIndices[1])
	{
		createInfoKHR.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfoKHR.queueFamilyIndexCount = 2;
		createInfoKHR.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfoKHR.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(pLogicalDevice->logicalDevice, &createInfoKHR, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: failed to create swapchain.\n";
		return;
	}

	vkGetDeviceQueue(pLogicalDevice->logicalDevice, queueFamilyIndices[1], 0, &m_presentQueue);

	std::vector<VkImage> swapchainImages(imageCount, VK_NULL_HANDLE);
	vkGetSwapchainImagesKHR(pLogicalDevice->logicalDevice, m_swapchain, &imageCount, swapchainImages.data());

	for (unsigned int i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = createInfo.surfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView swapchainImageView = VK_NULL_HANDLE;
		if (pDrawingDevice->CreateImageView(pLogicalDevice, imageViewCreateInfo, swapchainImageView) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: failed to create image view for swapchain image.\n";
			return;
		}

		m_renderTargets.emplace_back(std::make_shared<RenderTarget2D_Vulkan>(pLogicalDevice, swapchainImages[i], swapchainImageView, createInfo.swapExtent, createInfo.surfaceFormat.format));
	}
}

DrawingSwapchain_Vulkan::~DrawingSwapchain_Vulkan()
{
	m_renderTargets.clear();
	m_swapchainFrameBuffers.clear();

	vkDestroySwapchainKHR(m_pLogicalDevice->logicalDevice, m_swapchain, nullptr);
}

bool DrawingSwapchain_Vulkan::UpdateBackBuffer(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore, uint64_t timeout)
{
	return vkAcquireNextImageKHR(m_pLogicalDevice->logicalDevice, m_swapchain, timeout, pSemaphore->semaphore, VK_NULL_HANDLE, &m_frameBufferIndex) == VK_SUCCESS;
}

bool DrawingSwapchain_Vulkan::Present(const std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>>& waitSemaphores, uint32_t syncInterval)
{
	assert(m_swapchain != VK_NULL_HANDLE);
	assert(m_presentQueue != VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	std::vector<VkSemaphore> semaphoresToWait;
	for (auto& pSemaphore : waitSemaphores)
	{
		semaphoresToWait.emplace_back(pSemaphore->semaphore);
	}
	presentInfo.waitSemaphoreCount = (uint32_t)semaphoresToWait.size();
	presentInfo.pWaitSemaphores = semaphoresToWait.data();

	// TODO: we can present multiple swapchains at once if needed
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_frameBufferIndex;

	return vkQueuePresentKHR(m_presentQueue, &presentInfo) == VK_SUCCESS;
}

uint32_t DrawingSwapchain_Vulkan::GetTargetCount() const
{
	return (uint32_t)m_renderTargets.size();
}

std::shared_ptr<FrameBuffer_Vulkan> DrawingSwapchain_Vulkan::GetTargetFrameBuffer() const
{
	assert(m_frameBufferIndex < m_swapchainFrameBuffers.size());
	return m_swapchainFrameBuffers[m_frameBufferIndex];
}

RawShader_Vulkan::RawShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, const char* entry)
	: m_pDevice(pDevice), m_shaderModule(shaderModule), m_shaderStage(shaderStage), m_entryName(entry)
{

}

RawShader_Vulkan::~RawShader_Vulkan()
{
	if (m_shaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_pDevice->logicalDevice, m_shaderModule, nullptr);
	}

	m_rawCode.clear();
}

VertexShader_Vulkan::VertexShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry)
	: m_pShaderImpl(std::make_shared<RawShader_Vulkan>(pDevice, shaderModule, VK_SHADER_STAGE_VERTEX_BIT, entry))
{
	m_pShaderImpl->m_rawCode = rawCode;
}

std::shared_ptr<RawShader_Vulkan> VertexShader_Vulkan::GetShaderImpl() const
{
	return m_pShaderImpl;
}

FragmentShader_Vulkan::FragmentShader_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, VkShaderModule shaderModule, std::vector<char>& rawCode, const char* entry)
	: m_pShaderImpl(std::make_shared<RawShader_Vulkan>(pDevice, shaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, entry))
{
	m_pShaderImpl->m_rawCode = rawCode;
}

std::shared_ptr<RawShader_Vulkan> FragmentShader_Vulkan::GetShaderImpl() const
{
	return std::shared_ptr<RawShader_Vulkan>();
}

ShaderProgram_Vulkan::ShaderProgram_Vulkan(DrawingDevice_Vulkan* pDevice, uint32_t shaderCount, const std::shared_ptr<RawShader_Vulkan> pShader...)
	: ShaderProgram(0)
{
	m_pDevice = pDevice;

	va_list vaShaders;
	va_start(vaShaders, shaderCount); // shaderCount is the parameter preceding the first variable parameter 
	std::shared_ptr<RawShader_Vulkan> shaderPtr = nullptr;

	while (shaderCount > 0)
	{
		shaderPtr = va_arg(vaShaders, std::shared_ptr<RawShader_Vulkan>);
		ReflectResourceBinding(shaderPtr);		
		shaderCount--; // When there are no more arguments in ap, the behavior of va_arg is undefined, therefore we need to keep a count
	}

	va_end(vaShaders);
}

void ShaderProgram_Vulkan::ReflectResourceBinding(const std::shared_ptr<RawShader_Vulkan> pShader)
{
	size_t wordCount = pShader->m_rawCode.size() * sizeof(char) / sizeof(uint32_t);
	assert(wordCount > 0);
	std::vector<uint32_t> rawCode(wordCount);
	memcpy(rawCode.data(), pShader->m_rawCode.data(), pShader->m_rawCode.size() * sizeof(char));

	spirv_cross::Compiler spvCompiler(std::move(rawCode));

	spirv_cross::ShaderResources shaderRes = spvCompiler.get_shader_resources();

	// Query only active variables
	auto activeVars = spvCompiler.get_active_interface_variables();
	spvCompiler.set_enabled_interface_variables(std::move(activeVars));

#if defined(ENABLE_SHADER_REFLECT_OUTPUT_VK)
	std::cout << "SPIRV: Shader Stage: " << pShader->m_shaderStage << std::endl;
	std::cout << "Active Uniform Buffer Count: " << shaderRes.uniform_buffers.size() << std::endl;
	std::cout << "Active Push Constant Count: " << shaderRes.push_constant_buffers.size() << std::endl;
#endif

	for (auto& buffer : shaderRes.uniform_buffers)
	{
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::Uniform;
		desc.slot = spvCompiler.get_decoration(buffer.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(buffer.id).c_str(); //	Alert: this is incorrect for finding corresponding resource by name
		m_resourceTable.emplace(desc.name, desc);

		ProcessVariables(spvCompiler, buffer);
	}
}

void ShaderProgram_Vulkan::ProcessVariables(const spirv_cross::Compiler& spvCompiler, const spirv_cross::Resource& resource)
{

}

EShaderParamType ShaderProgram_Vulkan::GetParamType(const spirv_cross::SPIRType& type, uint32_t size)
{
	// Alert: we are over simplifying some details in here, e.g. data format and array info

	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Image:
	case spirv_cross::SPIRType::BaseType::SampledImage:
		return EShaderParamType::Texture2D;

	case spirv_cross::SPIRType::BaseType::Sampler:
		return EShaderParamType::Sampler;

	case spirv_cross::SPIRType::BaseType::Boolean:
	case spirv_cross::SPIRType::BaseType::SByte:
	case spirv_cross::SPIRType::BaseType::UByte:
	case spirv_cross::SPIRType::BaseType::Short:
	case spirv_cross::SPIRType::BaseType::UShort:
	case spirv_cross::SPIRType::BaseType::Half:
	case spirv_cross::SPIRType::BaseType::Int:
	case spirv_cross::SPIRType::BaseType::UInt:
	case spirv_cross::SPIRType::BaseType::Float:
	case spirv_cross::SPIRType::BaseType::Int64:
	case spirv_cross::SPIRType::BaseType::UInt64:
	case spirv_cross::SPIRType::BaseType::Double:
	{
		if (type.columns > 1)
		{
			// Alert: only square matrices are handled
			uint32_t rows = size / (type.columns * GetParamTypeSize(type));
			assert(type.columns == rows);
			switch (type.columns)
			{
			case 2U:
				return EShaderParamType::Mat2;

			case 3U:
				return EShaderParamType::Mat3;

			case 4U:
				return EShaderParamType::Mat4;

			default:
				throw std::runtime_error("Vulkan: unhandled irregular shader parameter matrix.");
				break;
			}
		}
		else if (type.vecsize > 1)
		{
			switch (type.vecsize)
			{
			case 2U:
				return EShaderParamType::Vec2;

			case 3U:
				return EShaderParamType::Vec3;

			case 4U:
				return EShaderParamType::Vec4;

			default:
				throw std::runtime_error("Vulkan: unhandled irregular shader parameter matrix.");
				break;
			}
		}
		else
		{
			// TODO: finish type conversion
		}
	}

	default:
		throw std::runtime_error("Vulkan: unhandled shader parameter type.");
		break;
	}

	return EShaderParamType::Invalid;
}

uint32_t ShaderProgram_Vulkan::GetParamTypeSize(const spirv_cross::SPIRType& type)
{
	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Boolean:
	case spirv_cross::SPIRType::BaseType::SByte:
	case spirv_cross::SPIRType::BaseType::UByte:
		return 1U;

	case spirv_cross::SPIRType::BaseType::Short:
	case spirv_cross::SPIRType::BaseType::UShort:
	case spirv_cross::SPIRType::BaseType::Half:
		return 2U;

	case spirv_cross::SPIRType::BaseType::Int:
	case spirv_cross::SPIRType::BaseType::UInt:
	case spirv_cross::SPIRType::BaseType::Float:
		return 4U;

	case spirv_cross::SPIRType::BaseType::Int64:
	case spirv_cross::SPIRType::BaseType::UInt64:
	case spirv_cross::SPIRType::BaseType::Double:
		return 8U;

	default:
		throw std::runtime_error("Vulkan: unhandled shader parameter type.");
		break;
	}

	return 0U;
}

EDataType ShaderProgram_Vulkan::BasicTypeConvert(const spirv_cross::SPIRType& type)
{
	switch (type.basetype)
	{
	case spirv_cross::SPIRType::BaseType::Boolean:
		return EDataType::Boolean;

	case spirv_cross::SPIRType::BaseType::Int:
		return EDataType::Int32;

	case spirv_cross::SPIRType::BaseType::UInt:
		return EDataType::UInt32;

	case spirv_cross::SPIRType::BaseType::Float:
		return EDataType::Float32;

	case spirv_cross::SPIRType::BaseType::Double:
		return EDataType::Double;

	case spirv_cross::SPIRType::BaseType::SByte:
		return EDataType::SByte;

	case spirv_cross::SPIRType::BaseType::UByte:
		return EDataType::UByte;

	case spirv_cross::SPIRType::BaseType::Short:
		return EDataType::Short;

	case spirv_cross::SPIRType::BaseType::UShort:
		return EDataType::UShort;

	case spirv_cross::SPIRType::BaseType::Half:
		return EDataType::Half;

	case spirv_cross::SPIRType::BaseType::Int64:
		return EDataType::Int64;

	case spirv_cross::SPIRType::BaseType::UInt64:
		return EDataType::UInt64;

	default:
		throw std::runtime_error("Vulkan: unhandled shader parameter base type.");
		break;
	}

	return EDataType::Float32;
}