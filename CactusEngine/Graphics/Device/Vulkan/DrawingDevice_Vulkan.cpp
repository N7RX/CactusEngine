#include "Global.h"
#include "DrawingDevice_Vulkan.h"
#include "DrawingResources_Vulkan.h"
#include "DrawingUtil_Vulkan.h"
#include "ImageTexture.h"
#include "RenderTexture.h"

#include <set>
#if defined(GLFW_IMPLEMENTATION_CACTUS)
#include <GLFW/glfw3.h>
#endif

using namespace Engine;

DrawingDevice_Vulkan::~DrawingDevice_Vulkan()
{
	if (m_isRunning)
	{
		ShutDown();
	}
}

void DrawingDevice_Vulkan::Initialize()
{
	SetupCommandManager();
	SetupUploadAllocator();
	SetupDescriptorAllocator();

	CreateDefaultSampler();

	m_currentFrame = 0;
	m_isRunning = true;
}

void DrawingDevice_Vulkan::ShutDown()
{
	if (m_isRunning)
	{
		// TODO: correctly organize the sequence of resource release

		m_isRunning = false;
	}
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
	tex2dCreateInfo.mipLevels = createInfo.generateMipmap ? DetermineMipmapLevels_VK(std::max<uint32_t>(createInfo.textureWidth, createInfo.textureHeight)) : 1;
	
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

	if (createInfo.generateMipmap)
	{
		pCmdBuffer->GenerateMipmap(std::static_pointer_cast<Texture2D_Vulkan>(pOutput));
	}

	pDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);

	if (createInfo.pSampler != nullptr)
	{
		pOutput->SetSampler(createInfo.pSampler);
	}

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

bool DrawingDevice_Vulkan::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput)
{
	UniformBufferCreateInfo_Vulkan vkUniformBufferCreateInfo = {};
	vkUniformBufferCreateInfo.size = createInfo.sizeInBytes;
	vkUniformBufferCreateInfo.appliedStages = VulkanShaderStageFlags(createInfo.appliedStages);
	vkUniformBufferCreateInfo.type = EUniformBufferType_Vulkan::Uniform;

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	pOutput = std::make_shared<UniformBuffer_Vulkan>(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1, vkUniformBufferCreateInfo);
#else
	pOutput = std::make_shared<FrameBuffer_Vulkan>(m_pDevice_0, vkUniformBufferCreateInfo);
#endif

	return pOutput != nullptr;
}

void DrawingDevice_Vulkan::ClearRenderTarget()
{
	std::cerr << "Vulkan: shouldn't call ClearRenderTarget on Vulkan device.\n";
}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments)
{
	std::cerr << "Vulkan: shouldn't call SetRenderTarget on Vulkan device.\n";
}

void DrawingDevice_Vulkan::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
{
	std::cerr << "Vulkan: shouldn't call SetRenderTarget on Vulkan device.\n";
}

void DrawingDevice_Vulkan::SetClearColor(Color4 color)
{
	m_clearValues.resize(2);
	m_clearValues[0].color = { color.r, color.g, color.b, color.a };
	m_clearValues[1].depthStencil = { 1.0f, 0 };
}

void DrawingDevice_Vulkan::SetBlendState(const DeviceBlendStateInfo& blendInfo)
{
	std::cerr << "Vulkan: shouldn't call SetBlendState on Vulkan device.\n";
}

void DrawingDevice_Vulkan::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable)
{
	auto pVkShader = std::static_pointer_cast<ShaderProgram_Vulkan>(pShaderProgram);

	std::vector<DesciptorUpdateInfo_Vulkan> updateInfos;

	for (auto& item : pTable->m_table)
	{
		DesciptorUpdateInfo_Vulkan updateInfo = {};
		updateInfo.hasContent = true;
		updateInfo.infoType = VulkanDescriptorResourceType(item.type);
		updateInfo.dstDescriptorType = VulkanDescriptorType(item.type);
		updateInfo.dstDescriptorBinding = item.binding;
		updateInfo.dstDescriptorSet = pVkShader->GetDescriptorSet(m_currentFrame);
		updateInfo.dstArrayElement = 0; // Alert: incorrect if it contains array

		switch (updateInfo.infoType)
		{
		case EDescriptorResourceType_Vulkan::Buffer:
		{
			auto pBuffer = std::static_pointer_cast<RawBuffer_Vulkan>(item.pResource);

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = pBuffer->m_buffer;
			bufferInfo.offset = 0; // Alert: incorrect if it contains sub-buffer division
			bufferInfo.range = VK_WHOLE_SIZE;

			updateInfo.bufferInfos.emplace_back(bufferInfo);

			break;
		}
		case EDescriptorResourceType_Vulkan::Image:
		{
			std::shared_ptr<Texture2D_Vulkan> pImage = nullptr;
			switch (std::static_pointer_cast<Texture2D>(item.pResource)->QuerySource())
			{
			case ETexture2DSource::ImageTexture:
				pImage = std::static_pointer_cast<Texture2D_Vulkan>(std::static_pointer_cast<ImageTexture>(item.pResource)->GetTexture());
				break;

			case ETexture2DSource::RenderTexture:
				pImage = std::static_pointer_cast<Texture2D_Vulkan>(std::static_pointer_cast<RenderTexture>(item.pResource)->GetTexture());
				break;

			case ETexture2DSource::RawDeviceTexture:
				pImage = std::static_pointer_cast<Texture2D_Vulkan>(item.pResource);
				break;

			default:
				throw std::runtime_error("Vulkan: Unhandled texture 2D source type.");
				return;
			}

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageView = pImage->m_imageView;
			imageInfo.imageLayout = pImage->m_layout;
			if (pImage->HasSampler())
			{
				imageInfo.sampler = std::static_pointer_cast<Sampler_Vulkan>(pImage->GetSampler())->m_sampler;
			}
			else
			{
				imageInfo.sampler = VK_NULL_HANDLE;
			}

			updateInfo.imageInfos.emplace_back(imageInfo);

			break;
		}
		case EDescriptorResourceType_Vulkan::TexelBuffer:
		{
			std::cerr << "Vulkan: TexelBuffer is unhandled.\n";
			updateInfo.hasContent = false;

			break;
		}
		default:
			std::cerr << "Vulkan: Unhandled descriptor resource type: " << (unsigned int)updateInfo.infoType << std::endl;
			updateInfo.hasContent = false;
			break;
		}

		updateInfos.emplace_back(updateInfo);
	}

	pVkShader->UpdateDescriptorSets(updateInfos);
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

	pCmdBuffer->DrawPrimitiveIndexed(indicesCount, 1, baseIndex, baseVertex);
}

void DrawingDevice_Vulkan::DrawFullScreenQuad()
{
	// Graphics pipelines should be properly setup in renderer, this function is only responsible for issuing draw call

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	auto pCmdBuffer = m_cmdGPUType == EGPUType::Discrete ? m_pDevice_0->pWorkingCmdBuffer : m_pDevice_1->pWorkingCmdBuffer;
#else
	auto pCmdBuffer = m_pDevice_0->pWorkingCmdBuffer;
#endif

	pCmdBuffer->DrawPrimitive(4, 1);
}

void DrawingDevice_Vulkan::ResizeViewPort(uint32_t width, uint32_t height)
{
	std::cerr << "Vulkan: shouldn't call ResizeViewPort on Vulkan device.\n";
}

EGraphicsDeviceType DrawingDevice_Vulkan::GetDeviceType() const
{
	return EGraphicsDeviceType::Vulkan;
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
		std::cerr << "Vulkan: Unhandled GPU type: " << (unsigned int)type << std::endl;
		return nullptr;
	}
#else
	return m_pDevice_0;
#endif
}

bool DrawingDevice_Vulkan::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput)
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	pOutput = std::make_shared<RenderPass_Vulkan>(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1);
#else
	pOutput = std::make_shared<RenderPass_Vulkan>(m_pDevice_0);
#endif

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

	return vkCreateRenderPass(std::static_pointer_cast<RenderPass_Vulkan>(pOutput)->m_pDevice->logicalDevice,
		&renderPassInfo, nullptr, &std::static_pointer_cast<RenderPass_Vulkan>(pOutput)->m_renderPass) == VK_SUCCESS;
}

bool DrawingDevice_Vulkan::CreateSampler(const TextureSamplerCreateInfo& createInfo, std::shared_ptr<TextureSampler>& pOutput)
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VulkanSamplerFilterMode(createInfo.magMode);
	samplerCreateInfo.minFilter = VulkanSamplerFilterMode(createInfo.minMode);
	samplerCreateInfo.mipmapMode = VulkanSamplerMipmapMode(createInfo.mipmapMode);
	samplerCreateInfo.addressModeU = VulkanSamplerAddressMode(createInfo.addressMode);
	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU; // Same address mode is applied to all axes
	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
	samplerCreateInfo.mipLodBias = createInfo.minLodBias;
	samplerCreateInfo.anisotropyEnable = createInfo.enableAnisotropy ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.maxAnisotropy = createInfo.maxAnisotropy;
	samplerCreateInfo.compareEnable = createInfo.enableCompareOp ? VK_TRUE : VK_FALSE;
	samplerCreateInfo.compareOp = VulkanCompareOp(createInfo.compareOp);
	samplerCreateInfo.minLod = createInfo.minLod;
	samplerCreateInfo.maxLod = createInfo.maxLod;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	pOutput = std::make_shared<Sampler_Vulkan>(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1, samplerCreateInfo);
#else
	pOutput = std::make_shared<Sampler_Vulkan>(m_pDevice_0, samplerCreateInfo);
#endif

	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, std::shared_ptr<PipelineVertexInputState>& pOutput)
{
	std::vector<VkVertexInputBindingDescription> bindingDescs;
	std::vector<VkVertexInputAttributeDescription> attributeDescs;

	for (auto& desc : createInfo.bindingDescs)
	{
		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.binding = desc.binding;
		bindingDesc.stride = desc.stride;
		bindingDesc.inputRate = VulkanVertexInputRate(desc.inputRate);

		bindingDescs.emplace_back(bindingDesc);
	}

	for (auto& desc : createInfo.attributeDescs)
	{
		VkVertexInputAttributeDescription attribDesc = {};
		attribDesc.location = desc.location;
		attribDesc.binding = desc.binding;
		attribDesc.offset = desc.offset;
		attribDesc.format = VulkanImageFormat(desc.format);

		attributeDescs.emplace_back(attribDesc);
	}

	pOutput = std::make_shared<PipelineVertexInputState_Vulkan>(bindingDescs, attributeDescs);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, std::shared_ptr<PipelineInputAssemblyState>& pOutput)
{
	pOutput = std::make_shared<PipelineInputAssemblyState_Vulkan>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, std::shared_ptr<PipelineColorBlendState>& pOutput)
{
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

	// If the independent blending feature is not enabled on the device, 
	// all VkPipelineColorBlendAttachmentState elements in the pAttachments array must be identical
	for (auto& desc : createInfo.blendStateDescs)
	{
		auto pTexture = std::static_pointer_cast<Texture2D_Vulkan>(desc.pAttachment);

		pTexture->m_pColorBlendAttachmentState->blendEnable = desc.enableBlend ? VK_TRUE : VK_FALSE;
		pTexture->m_pColorBlendAttachmentState = std::make_shared<VkPipelineColorBlendAttachmentState>();
		pTexture->m_pColorBlendAttachmentState->srcColorBlendFactor = VulkanBlendFactor(desc.srcColorBlendFactor);
		pTexture->m_pColorBlendAttachmentState->dstColorBlendFactor = VulkanBlendFactor(desc.dstColorBlendFactor);
		pTexture->m_pColorBlendAttachmentState->colorBlendOp = VulkanBlendOp(desc.colorBlendOp);
		pTexture->m_pColorBlendAttachmentState->srcAlphaBlendFactor = VulkanBlendFactor(desc.srcAlphaBlendFactor);
		pTexture->m_pColorBlendAttachmentState->dstAlphaBlendFactor = VulkanBlendFactor(desc.dstAlphaBlendFactor);
		pTexture->m_pColorBlendAttachmentState->alphaBlendOp = VulkanBlendOp(desc.alphaBlendOp);

		blendAttachmentStates.emplace_back(*pTexture->m_pColorBlendAttachmentState);
	}

	pOutput = std::make_shared<PipelineColorBlendState_Vulkan>(blendAttachmentStates);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, std::shared_ptr<PipelineRasterizationState>& pOutput)
{
	pOutput = std::make_shared<PipelineRasterizationState_Vulkan>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, std::shared_ptr<PipelineDepthStencilState>& pOutput)
{
	pOutput = std::make_shared<PipelineDepthStencilState_Vulkan>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, std::shared_ptr<PipelineMultisampleState>& pOutput)
{
	pOutput = std::make_shared<PipelineMultisampleState_Vulkan>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, std::shared_ptr<PipelineViewportState>& pOutput)
{
	pOutput = std::make_shared<PipelineViewportState_Vulkan>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_Vulkan::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput)
{
	auto pShaderProgram = std::static_pointer_cast<ShaderProgram_Vulkan>(createInfo.pShaderProgram);

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pushConstantRangeCount = pShaderProgram->GetPushConstantRangeCount();
	pipelineLayoutCreateInfo.pPushConstantRanges = pShaderProgram->GetPushConstantRanges();
	pipelineLayoutCreateInfo.pSetLayouts = pShaderProgram->GetDescriptorSetLayout()->GetDescriptorSetLayout();

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	if (vkCreatePipelineLayout(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0->logicalDevice : m_pDevice_1->logicalDevice,
		&pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
#else
	if (vkCreatePipelineLayout(m_pDevice_0->logicalDevice,
		&pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
#endif
	{
		throw std::runtime_error("Vulkan: failed to create graphics pipeline layout.\n");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineCreateInfo.stageCount = pShaderProgram->GetStageCount();
	pipelineCreateInfo.pStages = pShaderProgram->GetShaderStageCreateInfos();

	pipelineCreateInfo.pVertexInputState = std::static_pointer_cast<PipelineVertexInputState_Vulkan>(createInfo.pVertexInputState)->GetVertexInputStateCreateInfo();
	pipelineCreateInfo.pInputAssemblyState = std::static_pointer_cast<PipelineInputAssemblyState_Vulkan>(createInfo.pInputAssemblyState)->GetInputAssemblyStateCreateInfo();
	// ...(Tessellation State)
	pipelineCreateInfo.pViewportState = std::static_pointer_cast<PipelineViewportState_Vulkan>(createInfo.pViewportState)->GetViewportStateCreateInfo();
	pipelineCreateInfo.pRasterizationState = std::static_pointer_cast<PipelineRasterizationState_Vulkan>(createInfo.pRasterizationState)->GetRasterizationStateCreateInfo();
	pipelineCreateInfo.pMultisampleState = std::static_pointer_cast<PipelineMultisampleState_Vulkan>(createInfo.pMultisampleState)->GetMultisampleStateCreateInfo();
	if (createInfo.pColorBlendState) // Color blend state can be empty as it is attachment relevant
	{
		pipelineCreateInfo.pColorBlendState = std::static_pointer_cast<PipelineColorBlendState_Vulkan>(createInfo.pColorBlendState)->GetColorBlendStateCreateInfo();
	}
	pipelineCreateInfo.pDepthStencilState = std::static_pointer_cast<PipelineDepthStencilState_Vulkan>(createInfo.pDepthStencilState)->GetDepthStencilStateCreateInfo();
	// ...(Dynamics State)

	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = std::static_pointer_cast<RenderPass_Vulkan>(createInfo.pRenderPass)->m_renderPass;	
	//pipelineCreateInfo.subpass = createInfo.subpassIndex;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	pOutput = std::make_shared<GraphicsPipeline_Vulkan>(createInfo.deviceType == EGPUType::Discrete ? m_pDevice_0 : m_pDevice_1,
		pShaderProgram, pipelineCreateInfo);
#else
	pOutput = std::make_shared<GraphicsPipeline_Vulkan>(m_pDevice_0,
		pShaderProgram, pipelineCreateInfo);
#endif

	return pOutput != nullptr;
}

void DrawingDevice_Vulkan::SwitchCmdGPUContext(EGPUType type)
{
	m_cmdGPUType = type;
}

void DrawingDevice_Vulkan::TransitionImageLayout(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, EShaderType shaderStage)
{
	auto pVkImage = std::static_pointer_cast<Texture2D_Vulkan>(pImage);

	pVkImage->m_pDevice->pWorkingCmdBuffer->TransitionImageLayout(pVkImage, VulkanImageLayout(newLayout), shaderStage);
}

void DrawingDevice_Vulkan::TransitionImageLayout_Immediate(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, EShaderType shaderStage)
{
	auto pVkImage = std::static_pointer_cast<Texture2D_Vulkan>(pImage);

	auto pCmdBuffer = pVkImage->m_pDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
	pCmdBuffer->TransitionImageLayout(pVkImage, VulkanImageLayout(newLayout), shaderStage);

	pVkImage->m_pDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);
}

void DrawingDevice_Vulkan::ResizeSwapchain(uint32_t width, uint32_t height)
{
	// TODO: recreate swapchain
}

void DrawingDevice_Vulkan::BindGraphicsPipeline(const std::shared_ptr<GraphicsPipelineObject> pPipeline)
{
	auto pVkPipeline = std::static_pointer_cast<GraphicsPipeline_Vulkan>(pPipeline);

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	switch (m_cmdGPUType)
	{
	case EGPUType::Discrete:
		m_pDevice_0->pWorkingCmdBuffer->BindPipelineLayout(pVkPipeline->GetPipelineLayout());
		m_pDevice_0->pWorkingCmdBuffer->BindPipeline(pVkPipeline->GetBindPoint(), pVkPipeline->GetPipeline());
		break;

	case EGPUType::Integrated:
		m_pDevice_1->pWorkingCmdBuffer->BindPipelineLayout(pVkPipeline->GetPipelineLayout());
		m_pDevice_1->pWorkingCmdBuffer->BindPipeline(pVkPipeline->GetBindPoint(), pVkPipeline->GetPipeline());
		break;

	default:
		throw std::runtime_error("Vulkan: unhandled GPU type.\n");
	}
#else
	m_pDevice_0->pWorkingCmdBuffer->BindPipelineLayout(pVkPipeline->GetPipelineLayout());
	m_pDevice_0->pWorkingCmdBuffer->BindPipeline(pVkPipeline->GetBindPoint(), pVkPipeline->GetPipeline());
#endif
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

void DrawingDevice_Vulkan::EndRenderPass()
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	auto pCmdBuffer = m_cmdGPUType == EGPUType::Discrete ? m_pDevice_0->pWorkingCmdBuffer : m_pDevice_1->pWorkingCmdBuffer;
#else
	auto pCmdBuffer = m_pDevice_0->pWorkingCmdBuffer;
#endif
	assert(!pCmdBuffer->InRenderPass());

	pCmdBuffer->EndRenderPass();
}

void DrawingDevice_Vulkan::Present()
{
	assert(m_pSwapchain);

	// Present current frame

	auto pRenderFinishSemaphore = m_pSwapchain->m_pDevice->pSyncObjectManager->RequestSemaphore();
	m_pSwapchain->m_pDevice->pWorkingCmdBuffer->SignalSemaphore(pRenderFinishSemaphore);

	m_pSwapchain->m_pDevice->pGraphicsCommandManager->WaitFrameFenceSubmission();

	auto pFrameFence = m_pSwapchain->m_pDevice->pSyncObjectManager->RequestFence();
	m_pSwapchain->m_pDevice->pGraphicsCommandManager->SubmitCommandBuffers(pFrameFence->fence);

	if (vkWaitForFences(m_pSwapchain->m_pDevice->logicalDevice, 1, &pFrameFence->fence, VK_TRUE, FRAME_TIMEOUT) != VK_SUCCESS)
	{
		std::cerr << "Vulkan: frame timeout.\n";
	}

	std::vector<std::shared_ptr<DrawingSemaphore_Vulkan>> presentWaitSemaphores = { pRenderFinishSemaphore };
	m_pSwapchain->Present(presentWaitSemaphores);

	m_pSwapchain->m_pDevice->pSyncObjectManager->ReturnSemaphore(pRenderFinishSemaphore);

	// Preparation for next frame

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_IN_FLIGHT;

	m_pSwapchain->UpdateBackBuffer(m_currentFrame);
}

void DrawingDevice_Vulkan::FlushCommands(bool waitExecution)
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	if (!waitExecution)
	{
		m_pDevice_0->pGraphicsCommandManager->SubmitCommandBuffers(VK_NULL_HANDLE);
		m_pDevice_0->pWorkingCmdBuffer = m_pDevice_0->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		m_pDevice_1->pGraphicsCommandManager->SubmitCommandBuffers(VK_NULL_HANDLE);
		m_pDevice_1->pWorkingCmdBuffer = m_pDevice_1->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
	}
	else
	{
		auto pFence_0 = m_pDevice_0->pSyncObjectManager->RequestFence();
		m_pDevice_0->pGraphicsCommandManager->SubmitCommandBuffers(pFence_0->fence);
		m_pDevice_0->pWorkingCmdBuffer = m_pDevice_0->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		auto pFence_1 = m_pDevice_1->pSyncObjectManager->RequestFence();
		m_pDevice_1->pGraphicsCommandManager->SubmitCommandBuffers(pFence_1->fence);
		m_pDevice_1->pWorkingCmdBuffer = m_pDevice_1->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		if (vkWaitForFences(m_pDevice_0->logicalDevice, 1, &pFence_0->fence, VK_TRUE, FRAME_TIMEOUT) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: frame timeout.\n";
		}

		if (vkWaitForFences(m_pDevice_1->logicalDevice, 1, &pFence_1->fence, VK_TRUE, FRAME_TIMEOUT) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: frame timeout.\n";
		}
	}
#else
	if (!waitExecution)
	{
		m_pDevice_0->pGraphicsCommandManager->SubmitCommandBuffers(VK_NULL_HANDLE);
		m_pDevice_0->pWorkingCmdBuffer = m_pDevice_0->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
	}
	else
	{
		auto pFence_0 = m_pDevice_0->pSyncObjectManager->RequestFence();
		m_pDevice_0->pGraphicsCommandManager->SubmitCommandBuffers(pFence_0->fence);
		m_pDevice_0->pWorkingCmdBuffer = m_pDevice_0->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		if (vkWaitForFences(m_pDevice_0->logicalDevice, 1, &pFence_0->fence, VK_TRUE, FRAME_TIMEOUT) != VK_SUCCESS)
		{
			std::cerr << "Vulkan: frame timeout.\n";
		}
	}
#endif
}

std::shared_ptr<TextureSampler> DrawingDevice_Vulkan::GetDefaultTextureSampler(EGPUType deviceType) const
{
#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	return deviceType == EGPUType::Discrete ? m_pDefaultSampler_0 : m_pDefaultSampler_1;
#else
	return m_pDefaultSampler_0;
#endif
}

void DrawingDevice_Vulkan::GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const
{
	assert(m_pSwapchain != nullptr);

	uint32_t count = m_pSwapchain->GetSwapchainImageCount();

	for (uint32_t i = 0; i < count; i++)
	{
		outImages.emplace_back(m_pSwapchain->GetSwapchainImageByIndex(i));
	}
}

uint32_t DrawingDevice_Vulkan::GetSwapchainPresentImageIndex() const
{
	assert(m_pSwapchain != nullptr);
	return m_pSwapchain->GetTargetImageIndex();
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

void DrawingDevice_Vulkan::SetupSwapchain()
{
	DrawingSwapchainCreateInfo_Vulkan createInfo = {};
	createInfo.maxFramesInFlight = MAX_FRAME_IN_FLIGHT;
	createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // TODO: alter present mode according to vsync configuration
	createInfo.queueFamilyIndices = FindQueueFamilies_VK(m_pDevice_0->physicalDevice, m_presentationSurface);
	createInfo.supportDetails = QuerySwapchainSupport_VK(m_pDevice_0->physicalDevice, m_presentationSurface);
	createInfo.surface = m_presentationSurface;
	createInfo.surfaceFormat = ChooseSwapSurfaceFormat(createInfo.supportDetails.formats);
	createInfo.swapExtent = { gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth(),
		gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight() };

	m_pSwapchain = std::make_shared<DrawingSwapchain_Vulkan>(m_pDevice_0, createInfo);

	// TODO: add support for integrated GPU swapchain creation
}

VkSurfaceFormatKHR DrawingDevice_Vulkan::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return { VK_FORMAT_UNDEFINED , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }; // Alert: this could be incorrect
}

void DrawingDevice_Vulkan::CreateDefaultSampler()
{
	TextureSamplerCreateInfo createInfo = {};
	createInfo.deviceType = EGPUType::Discrete;
	createInfo.magMode = ESamplerFilterMode::Linear;
	createInfo.minMode = ESamplerFilterMode::Linear;
	createInfo.addressMode = ESamplerAddressMode::Repeat;
	createInfo.enableAnisotropy = false; // TODO: enable required extensions
	createInfo.maxAnisotropy = 16;
	createInfo.enableCompareOp = false;
	createInfo.compareOp = ECompareOperation::Always;
	createInfo.mipmapMode = ESamplerMipmapMode::Linear;
	createInfo.minLod = 0;
	createInfo.maxLod = 12.0f; // Support up to 4096
	createInfo.minLodBias = 0;

	std::shared_ptr<TextureSampler> pSampler_0;
	CreateSampler(createInfo, pSampler_0);
	m_pDefaultSampler_0 = std::static_pointer_cast<Sampler_Vulkan>(pSampler_0);

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	createInfo.deviceType = EGPUType::Integrated;

	std::shared_ptr<TextureSampler> pSampler_1;
	CreateSampler(createInfo, pSampler_1);
	m_pDefaultSampler_1 = std::static_pointer_cast<Sampler_Vulkan>(pSampler_1);
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
	m_pDevice_0->pWorkingCmdBuffer = m_pDevice_0->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
#if defined(ENABLE_COPY_QUEUE_VK)
	m_pDevice_0->pCopyCommandManager = std::make_shared<DrawingCommandManager_Vulkan>(m_pDevice_0, m_pDevice_0->copyQueue);
#endif

#if defined(ENABLE_HETEROGENEOUS_GPUS_VK)
	m_pDevice_1->pGraphicsCommandManager = std::make_shared<DrawingCommandManager_Vulkan>(m_pDevice_1, m_pDevice_1->graphicsQueue);
	m_pDevice_1->pWorkingCmdBuffer = m_pDevice_1->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
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
}

EDescriptorResourceType_Vulkan DrawingDevice_Vulkan::VulkanDescriptorResourceType(EDescriptorType type) const
{
	switch (type)
	{
	case EDescriptorType::UniformBuffer:
	case EDescriptorType::StorageBuffer:
		return EDescriptorResourceType_Vulkan::Buffer;

	case EDescriptorType::SampledImage:
	case EDescriptorType::CombinedImageSampler:
	case EDescriptorType::Sampler: // Alert: could be wrong to classify as image
	case EDescriptorType::StorageImage:
		return EDescriptorResourceType_Vulkan::Image;

	case EDescriptorType::UniformTexelBuffer:
	case EDescriptorType::StorageTexelBuffer:
		return EDescriptorResourceType_Vulkan::TexelBuffer;

	default:
		std::cerr << "Vulkan: Unhandled descriptor type: " << (unsigned int)type << std::endl;
		return EDescriptorResourceType_Vulkan::Buffer;
	}
}