#include "DrawingResources_Vulkan.h"
#include "DrawingDevice_Vulkan.h"
#include "BuiltInShaderType.h"

#include <cstdarg>

using namespace Engine;

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

VertexBuffer_Vulkan::VertexBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const RawBufferCreateInfo_Vulkan& vertexBufferCreateInfo, const RawBufferCreateInfo_Vulkan& indexBufferCreateInfo)
	: m_pDevice(pDevice)
{
	m_pVertexBufferImpl = std::make_shared<RawBuffer_Vulkan>(pDevice, vertexBufferCreateInfo);	

	m_pIndexBufferImpl = std::make_shared<RawBuffer_Vulkan>(pDevice, indexBufferCreateInfo);
	m_indexType = indexBufferCreateInfo.indexFormat;

	m_sizeInBytes = vertexBufferCreateInfo.size + indexBufferCreateInfo.size;
}

std::shared_ptr<RawBuffer_Vulkan> VertexBuffer_Vulkan::GetBufferImpl() const
{
	return m_pVertexBufferImpl;
}

std::shared_ptr<RawBuffer_Vulkan> VertexBuffer_Vulkan::GetIndexBufferImpl() const
{
	return m_pIndexBufferImpl;
}

VkIndexType VertexBuffer_Vulkan::GetIndexFormat() const
{
	return m_indexType;
}

Texture2D_Vulkan::Texture2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const Texture2DCreateInfo_Vulkan& createInfo)
	: m_pDevice(pDevice), m_allocatorType(EAllocatorType_Vulkan::VMA)
{
	assert(pDevice);

	m_pDevice->pUploadAllocator->CreateTexture2D(createInfo, *this);

	m_format = createInfo.format;
	m_extent = createInfo.extent;
	m_mipLevels = createInfo.mipLevels;
	m_aspect = createInfo.aspect;

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

	if (vkCreateImageView(m_pDevice->logicalDevice, &viewCreateInfo, nullptr, &m_imageView) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: failed to create image view for texture 2D.\n";
	}
}

Texture2D_Vulkan::~Texture2D_Vulkan()
{
	if (m_allocatorType == EAllocatorType_Vulkan::VMA)
	{
		m_pDevice->pUploadAllocator->FreeImage(m_image, m_allocation);
	}

	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pDevice->logicalDevice, m_imageView, nullptr);
	}
}

RenderTarget2D_Vulkan::RenderTarget2D_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const VkImage targetImage, const VkImageView targetView, const VkExtent2D& targetExtent, const VkFormat targetFormat)
{
	m_pDevice = pDevice;

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
		vkDestroyImage(m_pDevice->logicalDevice, m_image, nullptr);
	}
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pDevice->logicalDevice, m_imageView, nullptr);
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

FrameBuffer_Vulkan::FrameBuffer_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice)
	: m_pDevice(pDevice)
{

}

FrameBuffer_Vulkan::~FrameBuffer_Vulkan()
{
	vkDestroyFramebuffer(m_pDevice->logicalDevice, m_frameBuffer, nullptr);
}

uint32_t FrameBuffer_Vulkan::GetFrameBufferID() const
{
	return -1; // Vulkan device does not make use of this ID
}

DrawingSwapchain_Vulkan::DrawingSwapchain_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const DrawingSwapchainCreateInfo_Vulkan& createInfo)
	: m_pDevice(pDevice), m_presentQueue(VK_NULL_HANDLE), m_targetImageIndex(0)
{
	uint32_t imageCount = createInfo.supportDetails.capabilities.minImageCount - 1 + createInfo.maxFramesInFlight;
	if (createInfo.supportDetails.capabilities.maxImageCount > 0 && imageCount > createInfo.supportDetails.capabilities.maxImageCount)
	{
		imageCount = createInfo.supportDetails.capabilities.maxImageCount;
	}

	m_swapExtent = createInfo.swapExtent;

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

	if (vkCreateSwapchainKHR(m_pDevice->logicalDevice, &createInfoKHR, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: failed to create swapchain.\n";
		return;
	}

	vkGetDeviceQueue(m_pDevice->logicalDevice, queueFamilyIndices[1], 0, &m_presentQueue);

	std::vector<VkImage> swapchainImages(imageCount, VK_NULL_HANDLE);
	vkGetSwapchainImagesKHR(m_pDevice->logicalDevice, m_swapchain, &imageCount, swapchainImages.data());

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
		if (vkCreateImageView(m_pDevice->logicalDevice, &imageViewCreateInfo, nullptr, &swapchainImageView) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: failed to create image view for swapchain image.\n";
			return;
		}

		m_renderTargets.emplace_back(std::make_shared<RenderTarget2D_Vulkan>(m_pDevice, swapchainImages[i], swapchainImageView, createInfo.swapExtent, createInfo.surfaceFormat.format));
	}
}

DrawingSwapchain_Vulkan::~DrawingSwapchain_Vulkan()
{
	m_renderTargets.clear();

	vkDestroySwapchainKHR(m_pDevice->logicalDevice, m_swapchain, nullptr);
}

bool DrawingSwapchain_Vulkan::UpdateBackBuffer(std::shared_ptr<DrawingSemaphore_Vulkan> pSemaphore, uint64_t timeout)
{
	return vkAcquireNextImageKHR(m_pDevice->logicalDevice, m_swapchain, timeout, pSemaphore->semaphore, VK_NULL_HANDLE, &m_targetImageIndex) == VK_SUCCESS;
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
	presentInfo.pImageIndices = &m_targetImageIndex;

	return vkQueuePresentKHR(m_presentQueue, &presentInfo) == VK_SUCCESS;
}

uint32_t DrawingSwapchain_Vulkan::GetSwapchainImageCount() const
{
	return (uint32_t)m_renderTargets.size();
}

std::shared_ptr<RenderTarget2D_Vulkan> DrawingSwapchain_Vulkan::GetTargetImage() const
{
	assert(m_targetImageIndex < m_renderTargets.size());
	return m_renderTargets[m_targetImageIndex];
}

VkExtent2D DrawingSwapchain_Vulkan::GetSwapExtent() const
{
	return m_swapExtent;
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

ShaderProgram_Vulkan::ShaderProgram_Vulkan(DrawingDevice_Vulkan* pDevice, const std::shared_ptr<LogicalDevice_Vulkan> pLogicalDevice, uint32_t shaderCount, const std::shared_ptr<RawShader_Vulkan> pShader...)
	: ShaderProgram(0)
{
	m_pDevice = pDevice;

	va_list vaShaders;
	va_start(vaShaders, shaderCount); // shaderCount is the parameter preceding the first variable parameter 
	std::shared_ptr<RawShader_Vulkan> shaderPtr = nullptr;

	uint32_t shaderStages = 0;
	while (shaderCount > 0)
	{
		shaderPtr = va_arg(vaShaders, std::shared_ptr<RawShader_Vulkan>);
		ReflectResources(shaderPtr);
		shaderStages |= (uint32_t)ShaderStageBitsConvert(shaderPtr->m_shaderStage);

		shaderCount--; // When there are no more arguments in ap, the behavior of va_arg is undefined, therefore we need to keep a count
	}

	m_shaderStages = shaderStages;
	va_end(vaShaders);
}

void ShaderProgram_Vulkan::ReflectResources(const std::shared_ptr<RawShader_Vulkan> pShader)
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
	std::cout << "Sampled Image Count: " << shaderRes.sampled_images.size() << std::endl;
	std::cout << "Separate Image Count: " << shaderRes.separate_images.size() << std::endl;
	std::cout << "Separate Sample Count: " << shaderRes.separate_samplers.size() << std::endl;
#endif

	// TODO: replace this as input parameter
	uint32_t maxDescSetsCount = 32;

	LoadResourceBinding(spvCompiler, shaderRes);
	LoadResourceDescriptor(spvCompiler, shaderRes, ShaderStageBitsConvert(pShader->m_shaderStage), maxDescSetsCount);
}

void ShaderProgram_Vulkan::ProcessVariables(const spirv_cross::Compiler& spvCompiler, const spirv_cross::Resource& resource)
{
	auto resType = spvCompiler.get_type(resource.base_type_id);
	uint32_t memberCount = (uint32_t)resType.member_types.size();
	uint32_t resSize = (uint32_t)spvCompiler.get_declared_struct_size(resType);

	for (uint32_t i = 0; i < memberCount; ++i)
	{
		VariableDescription desc = {};

		desc.variableName = spvCompiler.get_member_name(resource.base_type_id, i).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*
		desc.uniformName = spvCompiler.get_name(resource.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*
		desc.offset = spvCompiler.type_struct_member_offset(resType, i);
		desc.variableSize = (uint32_t)spvCompiler.get_declared_struct_member_size(resType, i);
		desc.uniformSize = resSize;

		auto varType = spvCompiler.get_type(resType.member_types[i]);
		uint32_t varSize = i < (memberCount - 1) ? (spvCompiler.type_struct_member_offset(resType, i) - desc.offset) : (resSize - desc.offset);

		desc.paramType = GetParamType(varType, varSize);

		m_variableTable.emplace(desc.variableName, desc);
	}
}

void ShaderProgram_Vulkan::LoadResourceBinding(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes)
{
	for (auto& buffer : shaderRes.uniform_buffers)
	{
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::Uniform;
		desc.slot = spvCompiler.get_decoration(buffer.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(buffer.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*

		m_resourceTable.emplace(desc.name, desc);
		ProcessVariables(spvCompiler, buffer);
	}

#if defined(_DEBUG)
	uint32_t accumulatePushConstSize = 0;
#endif
	for (auto& constant : shaderRes.push_constant_buffers)
	{
#if defined(_DEBUG)
		accumulatePushConstSize += (uint32_t)spvCompiler.get_declared_struct_size(spvCompiler.get_type(constant.base_type_id));
#endif	
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::PushConstant;
		desc.slot = spvCompiler.get_decoration(constant.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(constant.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*

		m_resourceTable.emplace(desc.name, desc);
		ProcessVariables(spvCompiler, constant);
	}
#if defined(_DEBUG)
	assert(accumulatePushConstSize < m_pLogicalDevice->deviceProperties.limits.maxPushConstantsSize);
#endif

	for (auto& separateImage : shaderRes.separate_images)
	{
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::SeparateImage;
		desc.slot = spvCompiler.get_decoration(separateImage.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(separateImage.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*

		m_resourceTable.emplace(desc.name, desc);
	}

	for (auto& separateSampler : shaderRes.separate_samplers)
	{
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::SeparateSampler;
		desc.slot = spvCompiler.get_decoration(separateSampler.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(separateSampler.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*

		m_resourceTable.emplace(desc.name, desc);
	}

	for (auto& sampledImage : shaderRes.sampled_images)
	{
		ResourceDescription desc = {};
		desc.type = EShaderResourceType_Vulkan::SampledImage;
		desc.slot = spvCompiler.get_decoration(sampledImage.id, spv::DecorationBinding);
		desc.name = spvCompiler.get_name(sampledImage.id).c_str(); // Alert: this is incorrect for finding corresponding resource by const char*

		m_resourceTable.emplace(desc.name, desc);
	}

	// TODO: handle storage buffers
	// TODO: handle storage textures
	// TODO: handle subpass inputs
	// TODO: handle acceleration structures
}

void ShaderProgram_Vulkan::LoadResourceDescriptor(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, uint32_t maxDescSetCount)
{
	DescriptorSetCreateInfo descSetCreateInfo = {};
	descSetCreateInfo.maxDescSetCount = maxDescSetCount;
	std::vector<VkPushConstantRange> pushConstantRanges;

	LoadUniformBuffer(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
	LoadSeparateSampler(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
	LoadSeparateImage(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
	LoadImageSampler(spvCompiler, shaderRes, shaderType, descSetCreateInfo);
	LoadPushConstantBuffer(spvCompiler, shaderRes, shaderType, pushConstantRanges);


}

void ShaderProgram_Vulkan::LoadUniformBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
{
	uint32_t count = 0;

	for (auto& buffer : shaderRes.uniform_buffers)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorCount = 1; // Alert: not sure if this is correct for uniform blocks
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the uniform is bind with multiple shader stages
		binding.binding = spvCompiler.get_decoration(buffer.id, spv::DecorationBinding);
		binding.pImmutableSamplers = nullptr;

		descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
		count++;
	}

	if (count > 0)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count; // Alert: this could be incorrect

		descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
	}
}

void ShaderProgram_Vulkan::LoadSeparateSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
{
	uint32_t count = 0;

	for (auto& sampler : shaderRes.separate_samplers)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorCount = 1; // Alert: not sure if this is correct for uniform blocks
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		binding.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the uniform is bind with multiple shader stages
		binding.binding = spvCompiler.get_decoration(sampler.id, spv::DecorationBinding);
		binding.pImmutableSamplers = nullptr;

		descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
		count++;
	}

	if (count > 0)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count; // Alert: this could be incorrect

		descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
	}
}

void ShaderProgram_Vulkan::LoadSeparateImage(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
{
	uint32_t count = 0;

	for (auto& image : shaderRes.separate_images)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorCount = 1; // Alert: not sure if this is correct for uniform blocks
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
		binding.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the uniform is bind with multiple shader stages
		binding.binding = spvCompiler.get_decoration(image.id, spv::DecorationBinding);
		binding.pImmutableSamplers = nullptr;

		descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
		count++;
	}

	if (count > 0)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count; // Alert: this could be incorrect

		descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
	}
}

void ShaderProgram_Vulkan::LoadImageSampler(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, DescriptorSetCreateInfo& descSetCreateInfo)
{
	uint32_t count = 0;

	for (auto& sampledImage : shaderRes.sampled_images)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorCount = 1; // Alert: not sure if this is correct for uniform blocks
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the uniform is bind with multiple shader stages
		binding.binding = spvCompiler.get_decoration(sampledImage.id, spv::DecorationBinding);
		binding.pImmutableSamplers = nullptr;

		descSetCreateInfo.descSetLayoutBindings.emplace_back(binding);
		count++;
	}

	if (count > 0)
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = descSetCreateInfo.maxDescSetCount * count; // Alert: this could be incorrect

		descSetCreateInfo.descSetPoolSizes.emplace_back(poolSize);
	}
}

void ShaderProgram_Vulkan::LoadPushConstantBuffer(const spirv_cross::Compiler& spvCompiler, const spirv_cross::ShaderResources& shaderRes, EShaderType shaderType, std::vector<VkPushConstantRange>& outRanges)
{
	for (auto& constant : shaderRes.push_constant_buffers)
	{
		VkPushConstantRange range = {};
		range.offset = spvCompiler.get_decoration(constant.id, spv::DecorationOffset);
		range.size = (uint32_t)spvCompiler.get_declared_struct_size(spvCompiler.get_type(constant.base_type_id));
		range.stageFlags = ShaderTypeConvertToStageBits(shaderType); // ERROR: this would be incorrect if the uniform is bind with multiple shader stages

		outRanges.emplace_back(range);
	}
}

void ShaderProgram_Vulkan::CreateDescriptorSetLayout(const DescriptorSetCreateInfo& descSetCreateInfo)
{
	m_pDescriptorSetLayout = std::make_shared<DrawingDescriptorSetLayout_Vulkan>(m_pLogicalDevice, descSetCreateInfo.descSetLayoutBindings);
}

void ShaderProgram_Vulkan::CreateDescriptorPool(const DescriptorSetCreateInfo& descSetCreateInfo)
{
	m_pDescriptorPool = m_pLogicalDevice->pDescriptorAllocator->CreateDescriptorPool(descSetCreateInfo.maxDescSetCount, descSetCreateInfo.descSetPoolSizes);
}

void ShaderProgram_Vulkan::AllocateDescriptorSet(uint32_t count)
{
	assert(m_pDescriptorPool);
	assert(m_descriptorSets.size() + count <= MAX_DESCRIPTOR_SET_COUNT);

	std::vector<VkDescriptorSetLayout> layouts(count, m_pDescriptorSetLayout->GetSetLayout());
	std::vector<VkDescriptorSet> newDescSets;

	m_pDescriptorPool->AllocateDescriptorSets(layouts, newDescSets);

	for (auto& descSet : newDescSets)
	{
		m_descriptorSets.emplace_back(descSet);
	}
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
			return EShaderParamType::Scalar;
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

EShaderType ShaderProgram_Vulkan::ShaderStageBitsConvert(VkShaderStageFlagBits vkShaderStageBits)
{
	switch (vkShaderStageBits)
	{
	case VK_SHADER_STAGE_VERTEX_BIT:
		return EShaderType::Vertex;

	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return EShaderType::TessControl;

	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return EShaderType::TessEvaluation;

	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return EShaderType::Geometry;

	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return EShaderType::Fragment;

	case VK_SHADER_STAGE_COMPUTE_BIT:
		return EShaderType::Compute;

	case VK_SHADER_STAGE_ALL_GRAPHICS:
	case VK_SHADER_STAGE_ALL:
	case VK_SHADER_STAGE_RAYGEN_BIT_NV:
	case VK_SHADER_STAGE_ANY_HIT_BIT_NV:
	case VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV:
	case VK_SHADER_STAGE_MISS_BIT_NV:
	case VK_SHADER_STAGE_INTERSECTION_BIT_NV:
	case VK_SHADER_STAGE_CALLABLE_BIT_NV:
	case VK_SHADER_STAGE_TASK_BIT_NV:
	case VK_SHADER_STAGE_MESH_BIT_NV:
	default:
		throw std::runtime_error("Vulkan: unhandled shader stage bit.");
		break;
	}

	return EShaderType::Vertex; // Alert: returning unhandled stage as vertex stage could cause problem
}

VkShaderStageFlagBits ShaderProgram_Vulkan::ShaderTypeConvertToStageBits(EShaderType shaderType)
{
	switch (shaderType)
	{
	case EShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;

	case EShaderType::TessControl:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

	case EShaderType::TessEvaluation:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	case EShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;

	case EShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;

	case EShaderType::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;

	default:
		throw std::runtime_error("Vulkan: unhandled shader type.");
		break;
	}

	return VK_SHADER_STAGE_VERTEX_BIT; // Alert: returning unhandled stage as vertex stage could cause problem
}

GraphicsPipeline_Vulkan::GraphicsPipeline_Vulkan(const std::shared_ptr<LogicalDevice_Vulkan> pDevice, const std::shared_ptr<ShaderProgram_Vulkan> pShaderProgram, VkGraphicsPipelineCreateInfo& createInfo)
	: m_pDevice(pDevice), m_pShaderProgram(pShaderProgram)
{
	// TODO: support creation from cache & batched creations to reduce startup time

	m_pipelineLayout = createInfo.layout;
	m_pipeline = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(pDevice->logicalDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: failed to create graphics pipeline.\n";
	}
}

GraphicsPipeline_Vulkan::~GraphicsPipeline_Vulkan()
{
	assert(m_pipeline != VK_NULL_HANDLE);
}

VkPipeline GraphicsPipeline_Vulkan::GetPipeline() const
{
	return m_pipeline;
}

VkPipelineLayout GraphicsPipeline_Vulkan::GetPipelineLayout() const
{
	return m_pipelineLayout;
}

PipelineVertexInputState_Vulkan::PipelineVertexInputState_Vulkan(const std::vector<VkVertexInputBindingDescription>& bindingDescs, const std::vector<VkVertexInputAttributeDescription>& attributeDescs)
	: m_vertexBindingDesc(bindingDescs), m_vertexAttributeDesc(attributeDescs)
{
	m_pipelineVertexInputStateCreateInfo = {};
	m_pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)m_vertexBindingDesc.size();
	m_pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = m_vertexBindingDesc.data();
	m_pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)m_vertexAttributeDesc.size();
	m_pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = m_vertexAttributeDesc.data();
}

const VkPipelineVertexInputStateCreateInfo* PipelineVertexInputState_Vulkan::GetVertexInputStateCreateInfo() const
{
	return &m_pipelineVertexInputStateCreateInfo;
}

PipelineColorBlendState_Vulkan::PipelineColorBlendState_Vulkan(const std::vector<VkPipelineColorBlendAttachmentState>& blendAttachmentStates)
	: m_colorBlendAttachmentStates(blendAttachmentStates)
{
	m_pipelineColorBlendStateCreateInfo = {};
	m_pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	m_pipelineColorBlendStateCreateInfo.attachmentCount = (uint32_t)m_colorBlendAttachmentStates.size();
	m_pipelineColorBlendStateCreateInfo.pAttachments = m_colorBlendAttachmentStates.data();
	// Alert: these values might need to change according to blend factor
	m_pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	m_pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	m_pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	m_pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
}

const VkPipelineColorBlendStateCreateInfo* PipelineColorBlendState_Vulkan::GetColorBlendStateCreateInfo() const
{
	return &m_pipelineColorBlendStateCreateInfo;
}

void PipelineColorBlendState_Vulkan::EnableLogicOperation()
{
	// Applied only for signed and unsigned integer and normalized integer framebuffers
	m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_TRUE;
}

void PipelineColorBlendState_Vulkan::DisableLogicOpeartion()
{
	m_pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
}

void PipelineColorBlendState_Vulkan::SetLogicOpeartion(VkLogicOp logicOp)
{
	m_pipelineColorBlendStateCreateInfo.logicOp = logicOp;
}

PipelineViewportState_Vulkan::PipelineViewportState_Vulkan(uint32_t width, uint32_t height)
{
	m_viewport = {};
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;
	m_viewport.width = (float)width;
	m_viewport.height = (float)height;
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { width, height };

	m_pipelineViewportStateCreateInfo = {};
	m_pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_pipelineViewportStateCreateInfo.viewportCount = 1;
	m_pipelineViewportStateCreateInfo.pViewports = &m_viewport;
	m_pipelineViewportStateCreateInfo.scissorCount = 1;
	m_pipelineViewportStateCreateInfo.pScissors = &m_scissor;
}

const VkPipelineViewportStateCreateInfo* PipelineViewportState_Vulkan::GetViewportStateCreateInfo() const
{
	return &m_pipelineViewportStateCreateInfo;
}