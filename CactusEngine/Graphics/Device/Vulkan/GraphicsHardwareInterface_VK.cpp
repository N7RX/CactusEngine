#include "Global.h"
#include "GraphicsHardwareInterface_VK.h"
#include "Buffers_VK.h"
#include "Textures_VK.h"
#include "Swapchain_VK.h"
#include "Pipelines_VK.h"
#include "Shaders_VK.h"
#include "GHIUtilities_VK.h"
#include "ImageTexture.h"
#include "RenderTexture.h"
#include "Timer.h"
#include "MemoryAllocator.h"

#include <set>
#if defined(GLFW_IMPLEMENTATION_CE)
#include <GLFW/glfw3.h>
#endif

namespace Engine
{
	GraphicsHardwareInterface_VK::GraphicsHardwareInterface_VK()
		: GraphicsDevice(),
		m_instance(VK_NULL_HANDLE),
		m_presentationSurface(VK_NULL_HANDLE),
		m_debugMessenger(VK_NULL_HANDLE),
		m_isRunning(false),
		m_appInfo{},
		m_pMainDevice(nullptr),
		m_pSwapchain(nullptr)
	{

	}

	GraphicsHardwareInterface_VK::~GraphicsHardwareInterface_VK()
	{
		if (m_isRunning)
		{
			ShutDown();
		}
	}

	void GraphicsHardwareInterface_VK::Initialize()
	{
		GraphicsDevice::Initialize();

		SetupCommandManager();
		SetupSyncObjectManager();
		SetupUploadAllocator();
		SetupDescriptorAllocator();

		SetupSwapchain();

		m_isRunning = true;
	}

	void GraphicsHardwareInterface_VK::ShutDown()
	{
		GraphicsDevice::ShutDown();

		if (m_isRunning)
		{
			// TODO: correctly organize the sequence of resource release
			// ...

			vkDestroyDevice(m_pMainDevice->logicalDevice, nullptr);

			if (m_enableValidationLayers)
			{
				DestroyDebugUtilMessengerEXT_VK(m_instance, &m_debugMessenger, nullptr);
			}

			vkDestroySurfaceKHR(m_instance, m_presentationSurface, nullptr);
			vkDestroyInstance(m_instance, nullptr);

			m_isRunning = false;
		}
	}

	EGraphicsAPIType GraphicsHardwareInterface_VK::GetGraphicsAPIType() const
	{
		return EGraphicsAPIType::Vulkan;
	}

	ShaderProgram* GraphicsHardwareInterface_VK::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
	{
		VkShaderModule vertexModule = VK_NULL_HANDLE;
		VkShaderModule fragmentModule = VK_NULL_HANDLE;

		std::vector<char> vertexRawCode;
		std::vector<char> fragmentRawCode;

		CreateShaderModuleFromFile(vertexShaderFilePath, m_pMainDevice, vertexModule, vertexRawCode);
		CreateShaderModuleFromFile(fragmentShaderFilePath, m_pMainDevice, fragmentModule, fragmentRawCode);

		VertexShader_VK* pVertexShader;
		CE_NEW(pVertexShader, VertexShader_VK, m_pMainDevice, vertexModule, vertexRawCode);
		FragmentShader_VK* pFragmentShader;
		CE_NEW(pFragmentShader, FragmentShader_VK, m_pMainDevice, fragmentModule, fragmentRawCode);

	#if defined(DEBUG_MODE_CE)
		ShaderProgram_VK* pShaderProgram;
		CE_NEW(pShaderProgram, ShaderProgram_VK, this, m_pMainDevice, 2, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());
	#else
		ShaderProgram_VK* pShaderProgram;
		CE_NEW(pShaderProgram, ShaderProgram_VK, this, m_pMainDevice, pVertexShader->GetShaderImpl(), pFragmentShader->GetShaderImpl());
	#endif

		return pShaderProgram;
	}

	bool GraphicsHardwareInterface_VK::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, VertexBuffer*& pOutput)
	{
		// We are using directly mapped buffer instead of staging buffer
		// TODO: use staging pool for discrete device and CPU_TO_GPU for integrated device

		RawBufferCreateInfo_VK vertexBufferCreateInfo{};
		vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vertexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		vertexBufferCreateInfo.size = sizeof(float) * createInfo.positionDataCount
			+ sizeof(float) * createInfo.normalDataCount
			+ sizeof(float) * createInfo.texcoordDataCount
			+ sizeof(float) * createInfo.tangentDataCount
			+ sizeof(float) * createInfo.bitangentDataCount;
		vertexBufferCreateInfo.stride = createInfo.interleavedStride * sizeof(float);

		RawBufferCreateInfo_VK indexBufferCreateInfo{};
		indexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		indexBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		indexBufferCreateInfo.size = sizeof(int) * createInfo.indexDataCount;
		indexBufferCreateInfo.indexFormat = VK_INDEX_TYPE_UINT32;

		// By default vertex data will be created on discrete device, since integrated device will only handle post processing
		// The alternative is to add a device specifier in VertexBufferCreateInfo
		CE_NEW(pOutput, VertexBuffer_VK, m_pMainDevice->pUploadAllocator, vertexBufferCreateInfo, indexBufferCreateInfo);

		std::vector<float> interleavedVertices = createInfo.ConvertToInterleavedData();
		void* ppIndexData;
		void* ppVertexData;

		RawBufferCreateInfo_VK vertexStagingBufferCreateInfo{};
		vertexStagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		vertexStagingBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
		vertexStagingBufferCreateInfo.size = vertexBufferCreateInfo.size;

		RawBuffer_VK* pVertexStagingBuffer;
		CE_NEW(pVertexStagingBuffer, RawBuffer_VK, m_pMainDevice->pUploadAllocator, vertexStagingBufferCreateInfo);

		m_pMainDevice->pUploadAllocator->MapMemory(pVertexStagingBuffer->m_allocation, &ppVertexData);
		memcpy(ppVertexData, interleavedVertices.data(), vertexBufferCreateInfo.size);
		m_pMainDevice->pUploadAllocator->UnmapMemory(pVertexStagingBuffer->m_allocation);

		RawBufferCreateInfo_VK indexStagingBufferCreateInfo{};
		indexStagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		indexStagingBufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
		indexStagingBufferCreateInfo.size = indexBufferCreateInfo.size;

		RawBuffer_VK* pIndexStagingBuffer;
		CE_NEW(pIndexStagingBuffer, RawBuffer_VK, m_pMainDevice->pUploadAllocator, indexStagingBufferCreateInfo);

		m_pMainDevice->pUploadAllocator->MapMemory(pIndexStagingBuffer->m_allocation, &ppIndexData);
		memcpy(ppIndexData, createInfo.pIndexData, indexBufferCreateInfo.size);
		m_pMainDevice->pUploadAllocator->UnmapMemory(pIndexStagingBuffer->m_allocation);

		auto pCmdBuffer = m_pMainDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		VkBufferCopy vertexBufferCopyRegion{};
		vertexBufferCopyRegion.srcOffset = 0;
		vertexBufferCopyRegion.dstOffset = 0;
		vertexBufferCopyRegion.size = vertexBufferCreateInfo.size;

		pCmdBuffer->CopyBufferToBuffer(pVertexStagingBuffer, ((VertexBuffer_VK*)pOutput)->GetBufferImpl(), vertexBufferCopyRegion);

		VkBufferCopy indexBufferCopyRegion{};
		indexBufferCopyRegion.srcOffset = 0;
		indexBufferCopyRegion.dstOffset = 0;
		indexBufferCopyRegion.size = indexBufferCreateInfo.size;

		pCmdBuffer->CopyBufferToBuffer(pIndexStagingBuffer, ((VertexBuffer_VK*)pOutput)->GetIndexBufferImpl(), indexBufferCopyRegion);

		m_pMainDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);

		CE_DELETE(pVertexStagingBuffer);
		CE_DELETE(pIndexStagingBuffer);

		return true;
	}

	bool GraphicsHardwareInterface_VK::CreateTexture2D(const Texture2DCreateInfo& createInfo, Texture2D*& pOutput)
	{
		Texture2DCreateInfo_VK tex2dCreateInfo{};
		tex2dCreateInfo.extent = { createInfo.textureWidth, createInfo.textureHeight };
		tex2dCreateInfo.format = VulkanImageFormat(createInfo.format);
		tex2dCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // Alert: TILING_OPTIMAL could be incompatible with certain formats on certain devices
		tex2dCreateInfo.usage = DetermineImageUsage_VK(createInfo.textureType);
		tex2dCreateInfo.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		tex2dCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		tex2dCreateInfo.aspect = createInfo.textureType == ETextureType::DepthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		tex2dCreateInfo.mipLevels = (createInfo.generateMipmap || createInfo.reserveMipmapMemory) ? DetermineMipmapLevels_VK(std::max<uint32_t>(createInfo.textureWidth, createInfo.textureHeight)) : 1;
	
		auto pDevice = m_pMainDevice;

		CE_NEW(pOutput, Texture2D_VK, pDevice, tex2dCreateInfo);
		auto pVkTexture2D = (Texture2D_VK*)pOutput;

		auto pCmdBuffer = pDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
		RawBuffer_VK* pStagingBuffer = nullptr;

		if (createInfo.pTextureData != nullptr)
		{
			RawBufferCreateInfo_VK bufferCreateInfo{};
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
			bufferCreateInfo.size = (VkDeviceSize)createInfo.textureWidth * createInfo.textureHeight * VulkanFormatUnitSize(createInfo.format);

			CE_NEW(pStagingBuffer, RawBuffer_VK, pDevice->pUploadAllocator, bufferCreateInfo);

			void* ppData;
			pDevice->pUploadAllocator->MapMemory(pStagingBuffer->m_allocation, &ppData);
			memcpy(ppData, createInfo.pTextureData, bufferCreateInfo.size);
			pDevice->pUploadAllocator->UnmapMemory(pStagingBuffer->m_allocation);

			std::vector<VkBufferImageCopy> copyRegions;
			// Only has one level of data
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;  // Tightly packed
			region.bufferImageHeight = 0;// Tightly packed
			region.imageSubresource.aspectMask = IsDepthFormat(createInfo.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { createInfo.textureWidth, createInfo.textureHeight, 1 };
			copyRegions.emplace_back(region);

			pCmdBuffer->TransitionImageLayout(pVkTexture2D, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);

			pCmdBuffer->CopyBufferToTexture2D(pStagingBuffer, pVkTexture2D, copyRegions);
		}

		if (createInfo.generateMipmap)
		{
			DEBUG_ASSERT_CE(createInfo.pTextureData != nullptr);

			// Layout transition for all levels are included
			pCmdBuffer->GenerateMipmap(pVkTexture2D, VulkanImageLayout(createInfo.initialLayout), (uint32_t)EShaderType::Fragment);
		}
		else if (createInfo.initialLayout != EImageLayout::Undefined)
		{
			pCmdBuffer->TransitionImageLayout(pVkTexture2D, VulkanImageLayout(createInfo.initialLayout), (uint32_t)EShaderType::Fragment); // Alert: the applied stages may not limit to fragment shader
		}

		pDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer); // Alert: could end up submitting an empty command buffer

		if (createInfo.pSampler != nullptr)
		{
			pOutput->SetSampler(createInfo.pSampler);
		}

		CE_SAFE_DELETE(pStagingBuffer);

		return true;
	}

	bool GraphicsHardwareInterface_VK::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, FrameBuffer*& pOutput)
	{
		DEBUG_ASSERT_CE(pOutput == nullptr);

		CE_NEW(pOutput, FrameBuffer_VK, m_pMainDevice);

		auto pFrameBuffer = (FrameBuffer_VK*)pOutput;

		std::vector<VkImageView> viewAttachments;
		for (const auto& pAttachment : createInfo.attachments)
		{
			viewAttachments.emplace_back(((Texture2D_VK*)pAttachment)->m_imageView);
			pFrameBuffer->m_bufferAttachments.emplace_back(((Texture2D_VK*)pAttachment));
		}

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = ((RenderPass_VK*)createInfo.pRenderPass)->m_renderPass;
		frameBufferInfo.attachmentCount = (uint32_t)viewAttachments.size();
		frameBufferInfo.pAttachments = viewAttachments.data();
		frameBufferInfo.width = createInfo.framebufferWidth;
		frameBufferInfo.height = createInfo.framebufferHeight;
		frameBufferInfo.layers = 1;

		pFrameBuffer->m_width = createInfo.framebufferWidth;
		pFrameBuffer->m_height = createInfo.framebufferHeight;

		return vkCreateFramebuffer(m_pMainDevice->logicalDevice, &frameBufferInfo, nullptr, &pFrameBuffer->m_frameBuffer) == VK_SUCCESS;
	}

	bool GraphicsHardwareInterface_VK::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, UniformBuffer*& pOutput)
	{
		DEBUG_ASSERT_MESSAGE_CE(createInfo.maxSubAllocationCount > 0, "Vulkan: UniformBufferCreateInfo.maxSubAllocationCount cannot be 0.");

		UniformBufferCreateInfo_VK vkUniformBufferCreateInfo{};
		vkUniformBufferCreateInfo.size = createInfo.sizeInBytes * createInfo.maxSubAllocationCount;
		vkUniformBufferCreateInfo.appliedStages = VulkanShaderStageFlags(createInfo.appliedStages);
		vkUniformBufferCreateInfo.type = EUniformBufferType_VK::Uniform;
		vkUniformBufferCreateInfo.requiresFlush = (createInfo.maxSubAllocationCount > 1);

		CE_NEW(pOutput, UniformBuffer_VK, m_pMainDevice->pUploadAllocator, vkUniformBufferCreateInfo);

		return pOutput != nullptr;
	}

	void GraphicsHardwareInterface_VK::GenerateMipmap(Texture2D* pTexture, GraphicsCommandBuffer* pCmdBuffer)
	{
		auto pTextureVK = (Texture2D_VK*)pTexture;
		auto pCmdBufferVK = (CommandBuffer_VK*)pCmdBuffer;

		DEBUG_ASSERT_CE(pTextureVK->m_layout != VK_IMAGE_LAYOUT_UNDEFINED);

		VkImageLayout oldLayout = pTextureVK->m_layout;
		uint32_t oldStages = pTextureVK->m_appliedStages;

		pCmdBufferVK->TransitionImageLayout(pTextureVK, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);
		pCmdBufferVK->GenerateMipmap(pTextureVK, oldLayout, oldStages);
	}

	void GraphicsHardwareInterface_VK::CopyTexture2D(Texture2D* pSrcTexture, Texture2D* pDstTexture, GraphicsCommandBuffer* pCmdBuffer)
	{
		auto pSrc = (Texture2D_VK*)pSrcTexture;
		auto pDst = (Texture2D_VK*)pDstTexture;
		auto pCmdBufferVK = (CommandBuffer_VK*)pCmdBuffer;
		DEBUG_ASSERT_MESSAGE_CE(pSrc->GetWidth() == pDst->GetWidth() && pSrc->GetHeight() == pDst->GetHeight(), "CopyTexture2D textures dimension(s) mismatch.");

		std::vector<VkImageCopy> copyRegions;
		VkImageCopy region{};
		region.srcOffset = { 0, 0, 0 };
		region.dstOffset = { 0, 0, 0 };
		region.extent = { pSrc->GetWidth(), pDst->GetHeight(), 1 };
		region.srcSubresource.aspectMask = (pSrc->GetTextureType() == ETextureType::DepthAttachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.dstSubresource.aspectMask = (pDst->GetTextureType() == ETextureType::DepthAttachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;
		copyRegions.emplace_back(region);
		// Mipmap levels are not copied; will need to update this function if needed

		VkImageLayout oldSrcLayout = pSrc->m_layout;
		VkImageLayout oldDstLayout = pDst->m_layout;
		uint32_t oldSrcStages = pSrc->m_appliedStages;
		uint32_t oldDstStages = pDst->m_appliedStages;

		pCmdBufferVK->TransitionImageLayout(pSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);
		pCmdBufferVK->TransitionImageLayout(pDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);
		pCmdBufferVK->CopyTexture2DToTexture2D(pSrc, pDst, copyRegions);
		pCmdBufferVK->TransitionImageLayout(pSrc, oldSrcLayout, oldSrcStages);
		pCmdBufferVK->TransitionImageLayout(pDst, oldDstLayout, oldDstStages);
	}

	bool GraphicsHardwareInterface_VK::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, RenderPassObject*& pOutput)
	{
		CE_NEW(pOutput, RenderPass_VK, m_pMainDevice);

		auto pRenderPass = (RenderPass_VK*)pOutput;

		std::vector<VkAttachmentDescription> attachmentDescs;
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		VkAttachmentReference depthAttachmentRef{};
		bool foundDepthAttachment = false;

		VkClearValue clearColor;
		clearColor.color = { createInfo.clearColor.r, createInfo.clearColor.g, createInfo.clearColor.b, createInfo.clearColor.a };

		VkClearValue clearDepthStencil;
		clearDepthStencil.depthStencil = { createInfo.clearDepth, (uint32_t)createInfo.clearStencil };

		for (uint32_t i = 0; i < createInfo.attachmentDescriptions.size(); i++)
		{
			VkAttachmentDescription desc{};
			desc.format = VulkanImageFormat(createInfo.attachmentDescriptions[i].format);
			desc.samples = VulkanSampleCount(createInfo.attachmentDescriptions[i].sampleCount);
			desc.loadOp = VulkanLoadOp(createInfo.attachmentDescriptions[i].loadOp);
			desc.storeOp = VulkanStoreOp(createInfo.attachmentDescriptions[i].storeOp);
			desc.stencilLoadOp = VulkanLoadOp(createInfo.attachmentDescriptions[i].stencilLoadOp);
			desc.stencilStoreOp = VulkanStoreOp(createInfo.attachmentDescriptions[i].stencilStoreOp);
			desc.initialLayout = VulkanImageLayout(createInfo.attachmentDescriptions[i].initialLayout);
			desc.finalLayout = VulkanImageLayout(createInfo.attachmentDescriptions[i].finalLayout);

			attachmentDescs.emplace_back(desc);

			if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Color)
			{
				VkAttachmentReference ref{};
				ref.attachment = createInfo.attachmentDescriptions[i].index;
				ref.layout = VulkanImageLayout(createInfo.attachmentDescriptions[i].usageLayout);

				colorAttachmentRefs.emplace_back(ref);

				pRenderPass->m_clearValues.emplace_back(clearColor);
			}
			else if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Depth)
			{
				DEBUG_ASSERT_CE(!foundDepthAttachment);
				foundDepthAttachment = true;

				depthAttachmentRef.attachment = createInfo.attachmentDescriptions[i].index;
				depthAttachmentRef.layout = VulkanImageLayout(createInfo.attachmentDescriptions[i].usageLayout);
			}
		}

		if (!foundDepthAttachment)
		{
			depthAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
		}
		else
		{
			pRenderPass->m_clearValues.emplace_back(clearDepthStencil);
		}

		// TODO: add support for multiple subpasses

		VkSubpassDescription subpassDesc{};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size();
		subpassDesc.pColorAttachments = colorAttachmentRefs.data();
		subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = (uint32_t)attachmentDescs.size();
		renderPassInfo.pAttachments = attachmentDescs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDesc;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(m_pMainDevice->logicalDevice, &renderPassInfo, nullptr, &pRenderPass->m_renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan: failed to create render pass.");
		}

		return true;
	}

	bool GraphicsHardwareInterface_VK::CreateSampler(const TextureSamplerCreateInfo& createInfo, TextureSampler*& pOutput)
	{
		VkSamplerCreateInfo samplerCreateInfo{};
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

		CE_NEW(pOutput, Sampler_VK, m_pMainDevice, samplerCreateInfo);

		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, PipelineVertexInputState*& pOutput)
	{
		std::vector<VkVertexInputBindingDescription> bindingDescs;
		std::vector<VkVertexInputAttributeDescription> attributeDescs;

		for (auto& desc : createInfo.bindingDescs)
		{
			VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.binding = desc.binding;
			bindingDesc.stride = desc.stride;
			bindingDesc.inputRate = VulkanVertexInputRate(desc.inputRate);

			bindingDescs.emplace_back(bindingDesc);
		}

		for (auto& desc : createInfo.attributeDescs)
		{
			VkVertexInputAttributeDescription attribDesc{};
			attribDesc.location = desc.location;
			attribDesc.binding = desc.binding;
			attribDesc.offset = desc.offset;
			attribDesc.format = VulkanImageFormat(desc.format);

			attributeDescs.emplace_back(attribDesc);
		}

		CE_NEW(pOutput, PipelineVertexInputState_VK, bindingDescs, attributeDescs);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, PipelineInputAssemblyState*& pOutput)
	{
		CE_NEW(pOutput, PipelineInputAssemblyState_VK, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, PipelineColorBlendState*& pOutput)
	{
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		// If the independent blending feature is not enabled on the device, 
		// all VkPipelineColorBlendAttachmentState elements in the pAttachments array must be identical
		for (auto& desc : createInfo.blendStateDescs)
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};

			colorBlendAttachmentState.blendEnable = desc.enableBlend ? VK_TRUE : VK_FALSE;
			colorBlendAttachmentState.srcColorBlendFactor = VulkanBlendFactor(desc.srcColorBlendFactor);
			colorBlendAttachmentState.dstColorBlendFactor = VulkanBlendFactor(desc.dstColorBlendFactor);
			colorBlendAttachmentState.colorBlendOp = VulkanBlendOp(desc.colorBlendOp);
			colorBlendAttachmentState.srcAlphaBlendFactor = VulkanBlendFactor(desc.srcAlphaBlendFactor);
			colorBlendAttachmentState.dstAlphaBlendFactor = VulkanBlendFactor(desc.dstAlphaBlendFactor);
			colorBlendAttachmentState.alphaBlendOp = VulkanBlendOp(desc.alphaBlendOp);
			colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			blendAttachmentStates.emplace_back(colorBlendAttachmentState);
		}

		CE_NEW(pOutput, PipelineColorBlendState_VK, blendAttachmentStates);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, PipelineRasterizationState*& pOutput)
	{
		CE_NEW(pOutput, PipelineRasterizationState_VK, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, PipelineDepthStencilState*& pOutput)
	{
		CE_NEW(pOutput, PipelineDepthStencilState_VK, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, PipelineMultisampleState*& pOutput)
	{
		CE_NEW(pOutput, PipelineMultisampleState_VK, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, PipelineViewportState*& pOutput)
	{
		CE_NEW(pOutput, PipelineViewportState_VK, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_VK::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipelineObject*& pOutput)
	{
		auto pShaderProgram = (ShaderProgram_VK*)createInfo.pShaderProgram;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pushConstantRangeCount = pShaderProgram->GetPushConstantRangeCount();
		pipelineLayoutCreateInfo.pPushConstantRanges = pShaderProgram->GetPushConstantRanges();
		pipelineLayoutCreateInfo.pSetLayouts = pShaderProgram->GetDescriptorSetLayout()->GetDescriptorSetLayout();

		if (vkCreatePipelineLayout(m_pMainDevice->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan: failed to create graphics pipeline layout.\n");
			return false;
		}

		std::vector<VkDynamicState> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, (uint32_t)dynamicStates.size(), dynamicStates.data() };

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		pipelineCreateInfo.stageCount = pShaderProgram->GetStageCount();
		pipelineCreateInfo.pStages = pShaderProgram->GetShaderStageCreateInfos();

		pipelineCreateInfo.pVertexInputState = ((PipelineVertexInputState_VK*)createInfo.pVertexInputState)->GetVertexInputStateCreateInfo();
		pipelineCreateInfo.pInputAssemblyState = ((PipelineInputAssemblyState_VK*)createInfo.pInputAssemblyState)->GetInputAssemblyStateCreateInfo();
		// (No tessellation state yet)
		pipelineCreateInfo.pViewportState = ((PipelineViewportState_VK*)createInfo.pViewportState)->GetViewportStateCreateInfo();
		pipelineCreateInfo.pRasterizationState = ((PipelineRasterizationState_VK*)createInfo.pRasterizationState)->GetRasterizationStateCreateInfo();
		pipelineCreateInfo.pMultisampleState = ((PipelineMultisampleState_VK*)createInfo.pMultisampleState)->GetMultisampleStateCreateInfo();
		pipelineCreateInfo.pColorBlendState = ((PipelineColorBlendState_VK*)createInfo.pColorBlendState)->GetColorBlendStateCreateInfo();
		pipelineCreateInfo.pDepthStencilState = ((PipelineDepthStencilState_VK*)createInfo.pDepthStencilState)->GetDepthStencilStateCreateInfo();
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = ((RenderPass_VK*)createInfo.pRenderPass)->m_renderPass;	
		//pipelineCreateInfo.subpass = createInfo.subpassIndex;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

		CE_NEW(pOutput, GraphicsPipeline_VK, m_pMainDevice, pShaderProgram, pipelineCreateInfo);

		if (pOutput != nullptr)
		{
			pOutput->UpdateViewportState(createInfo.pViewportState);
		}

		return pOutput != nullptr;
	}

	void GraphicsHardwareInterface_VK::TransitionImageLayout(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages)
	{
		TransitionImageLayout(m_pMainDevice->pImplicitCmdBuffer, pImage, newLayout, appliedStages);
	}

	void GraphicsHardwareInterface_VK::TransitionImageLayout(GraphicsCommandBuffer* pCommandBuffer, Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages)
	{
		Texture2D_VK* pVkImage = nullptr;
		switch (((Texture2D*)pImage)->QuerySource())
		{
		case ETexture2DSource::ImageTexture:
			pVkImage = (Texture2D_VK*)(((ImageTexture*)pImage)->GetTexture());
			break;

		case ETexture2DSource::RenderTexture:
			pVkImage = (Texture2D_VK*)(((RenderTexture*)pImage)->GetTexture());
			break;

		case ETexture2DSource::RawDeviceTexture:
			pVkImage = (Texture2D_VK*)pImage;
			break;

		default:
			throw std::runtime_error("Vulkan: Unhandled texture 2D source type.");
			return;
		}

		((CommandBuffer_VK*)pCommandBuffer)->TransitionImageLayout(pVkImage, VulkanImageLayout(newLayout), appliedStages);
	}

	void GraphicsHardwareInterface_VK::TransitionImageLayout_Immediate(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages)
	{
		Texture2D_VK* pVkImage = nullptr;
		switch (((Texture2D*)pImage)->QuerySource())
		{
		case ETexture2DSource::ImageTexture:
			pVkImage = (Texture2D_VK*)(((ImageTexture*)pImage)->GetTexture());
			break;

		case ETexture2DSource::RenderTexture:
			pVkImage = (Texture2D_VK*)(((RenderTexture*)pImage)->GetTexture());
			break;

		case ETexture2DSource::RawDeviceTexture:
			pVkImage = (Texture2D_VK*)pImage;
			break;

		default:
			throw std::runtime_error("Vulkan: Unhandled texture 2D source type.");
			return;
		}

		auto pCmdBuffer = m_pMainDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
		pCmdBuffer->TransitionImageLayout(pVkImage, VulkanImageLayout(newLayout), appliedStages);

		m_pMainDevice->pGraphicsCommandManager->SubmitSingleCommandBuffer_Immediate(pCmdBuffer);
	}

	GraphicsCommandPool* GraphicsHardwareInterface_VK::RequestExternalCommandPool(EQueueType queueType)
	{
		auto pDevice = m_pMainDevice;

		switch (queueType)
		{
		case EQueueType::Graphics:
		{
			return pDevice->pGraphicsCommandManager->RequestExternalCommandPool();
		}

		case EQueueType::Transfer:
		{
			if (pDevice->pTransferCommandManager != nullptr)
			{
				return pDevice->pTransferCommandManager->RequestExternalCommandPool();
			}
			else
			{
				LOG_WARNING("Vulkan: Transfer command pool requested but transfer queue is not supported.");
				return pDevice->pGraphicsCommandManager->RequestExternalCommandPool();
			}
		}
		default:
		{
			LOG_ERROR("Vulkan: Unhandled queue type: " + std::to_string((uint32_t)queueType));
			return nullptr;
		}
		}
	}

	GraphicsCommandBuffer* GraphicsHardwareInterface_VK::RequestCommandBuffer(GraphicsCommandPool* pCommandPool)
	{
		auto pCmdBuffer = ((CommandPool_VK*)pCommandPool)->RequestPrimaryCommandBuffer();
		pCmdBuffer->m_usageFlags = (uint32_t)ECommandBufferUsageFlagBits_VK::Explicit;
		pCmdBuffer->m_isExternal = true;
		return pCmdBuffer;
	}

	void GraphicsHardwareInterface_VK::ReturnExternalCommandBuffer(GraphicsCommandBuffer* pCommandBuffer)
	{
		((CommandBuffer_VK*)pCommandBuffer)->m_pAllocatedPool->m_pManager->ReturnExternalCommandBuffer((CommandBuffer_VK*)pCommandBuffer);
	}

	GraphicsSemaphore* GraphicsHardwareInterface_VK::RequestGraphicsSemaphore(ESemaphoreWaitStage waitStage)
	{
		auto pSemaphore = m_pMainDevice->pSyncObjectManager->RequestTimelineSemaphore();
		pSemaphore->waitStage = VulkanSemaphoreWaitStage(waitStage);
		return pSemaphore;
	}

	void GraphicsHardwareInterface_VK::BindGraphicsPipeline(const GraphicsPipelineObject* pPipeline, GraphicsCommandBuffer* pCommandBuffer)
	{
		auto pPipelineVK = (GraphicsPipeline_VK*)pPipeline;
		auto pCommandBufferVK = (CommandBuffer_VK*)pCommandBuffer;

		pCommandBufferVK->BindPipelineLayout(pPipelineVK->GetPipelineLayout());
		pCommandBufferVK->BindPipeline(pPipelineVK->GetBindPoint(), pPipelineVK->GetPipeline());
		pCommandBufferVK->SetViewport(pPipelineVK->GetViewport(), pPipelineVK->GetScissor());
	}

	void GraphicsHardwareInterface_VK::BeginRenderPass(const RenderPassObject* pRenderPass, const FrameBuffer* pFrameBuffer, GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(!((CommandBuffer_VK*)pCommandBuffer)->InRenderPass());
		// Currently we are only rendering to full window
		((CommandBuffer_VK*)pCommandBuffer)->BeginRenderPass(
			((RenderPass_VK*)pRenderPass)->m_renderPass,
			((FrameBuffer_VK*)pFrameBuffer)->m_frameBuffer,
			((RenderPass_VK*)pRenderPass)->m_clearValues, { pFrameBuffer->GetWidth(), pFrameBuffer->GetHeight() }
		);
	}

	void GraphicsHardwareInterface_VK::EndRenderPass(GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(((CommandBuffer_VK*)pCommandBuffer)->InRenderPass());
		((CommandBuffer_VK*)pCommandBuffer)->EndRenderPass();
	}

	void GraphicsHardwareInterface_VK::EndCommandBuffer(GraphicsCommandBuffer* pCommandBuffer)
	{
		((CommandBuffer_VK*)pCommandBuffer)->EndCommandBuffer();
	}

	void GraphicsHardwareInterface_VK::CommandWaitSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore)
	{
		if (pSemaphore == nullptr)
		{
			return;
		}
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		((CommandBuffer_VK*)pCommandBuffer)->WaitSemaphore((TimelineSemaphore_VK*)pSemaphore);
	}

	void GraphicsHardwareInterface_VK::CommandSignalSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore)
	{
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr && pSemaphore != nullptr);
		((CommandBuffer_VK*)pCommandBuffer)->SignalSemaphore((TimelineSemaphore_VK*)pSemaphore);
	}

	void GraphicsHardwareInterface_VK::UpdateShaderParameter(ShaderProgram* pShaderProgram, const ShaderParameterTable* pTable, GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		auto pVkShader = (ShaderProgram_VK*)pShaderProgram;

		std::vector<DesciptorUpdateInfo_VK> updateInfos;
		DescriptorSet_VK* pTargetDescriptorSet = pVkShader->GetDescriptorSet();

		for (auto& item : pTable->m_table)
		{
			DesciptorUpdateInfo_VK updateInfo{};
			updateInfo.hasContent = true;
			updateInfo.infoType = VulkanDescriptorResourceType(item.type);
			updateInfo.dstDescriptorType = VulkanDescriptorType(item.type);
			updateInfo.dstDescriptorBinding = item.binding;
			updateInfo.dstDescriptorSet = pTargetDescriptorSet->m_descriptorSet;
			updateInfo.dstArrayElement = 0; // Alert: incorrect if it contains array

			switch (updateInfo.infoType)
			{
			case EDescriptorResourceType_VK::Buffer:
			{
				VkDescriptorBufferInfo bufferInfo{};
				GetBufferInfoByDescriptorType(item.type, item.pResource, bufferInfo);

				updateInfo.bufferInfos.emplace_back(bufferInfo);

				break;
			}
			case EDescriptorResourceType_VK::Image:
			{
				Texture2D_VK* pImage = nullptr;
				switch (((Texture2D*)item.pResource)->QuerySource())
				{
				case ETexture2DSource::ImageTexture:
					pImage = (Texture2D_VK*)(((ImageTexture*)item.pResource)->GetTexture());
					break;

				case ETexture2DSource::RenderTexture:
					pImage = (Texture2D_VK*)(((RenderTexture*)item.pResource)->GetTexture());
					break;

				case ETexture2DSource::RawDeviceTexture:
					pImage = (Texture2D_VK*)item.pResource;
					break;

				default:
					throw std::runtime_error("Vulkan: Unhandled texture 2D source type.");
					return;
				}

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageView = pImage->m_imageView;
				imageInfo.imageLayout = pImage->m_layout;
				if (pImage->HasSampler())
				{
					imageInfo.sampler = ((Sampler_VK*)pImage->GetSampler())->m_sampler;
				}
				else
				{
					imageInfo.sampler = VK_NULL_HANDLE;
				}

				updateInfo.imageInfos.emplace_back(imageInfo);

				break;
			}
			case EDescriptorResourceType_VK::TexelBuffer:
			{
				LOG_ERROR("Vulkan: TexelBuffer is unhandled.");
				updateInfo.hasContent = false;

				break;
			}
			default:
				LOG_ERROR("Vulkan: Unhandled descriptor resource type: " + std::to_string((uint32_t)updateInfo.infoType));
				updateInfo.hasContent = false;
				break;
			}

			updateInfos.emplace_back(updateInfo);
		}

		pVkShader->UpdateDescriptorSets(updateInfos);

		std::vector<DescriptorSet_VK*> descSets = { pTargetDescriptorSet };
		((CommandBuffer_VK*)pCommandBuffer)->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, descSets);
	}

	void GraphicsHardwareInterface_VK::SetVertexBuffer(const VertexBuffer* pVertexBuffer, GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		auto pBuffer = (VertexBuffer_VK*)pVertexBuffer;

		static VkDeviceSize defaultOffset = 0;

		((CommandBuffer_VK*)pCommandBuffer)->BindVertexBuffer(0, 1, &pBuffer->GetBufferImpl()->m_buffer, &defaultOffset);
		((CommandBuffer_VK*)pCommandBuffer)->BindIndexBuffer(pBuffer->GetIndexBufferImpl()->m_buffer, 0, pBuffer->GetIndexFormat());
	}

	void GraphicsHardwareInterface_VK::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		((CommandBuffer_VK*)pCommandBuffer)->DrawPrimitiveIndexed(indicesCount, 1, baseIndex, baseVertex);
	}

	void GraphicsHardwareInterface_VK::DrawFullScreenQuad(GraphicsCommandBuffer* pCommandBuffer)
	{
		// Graphics pipelines should be properly setup in renderer, this function is only responsible for issuing draw call
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		((CommandBuffer_VK*)pCommandBuffer)->DrawPrimitive(4, 1);
	}

	void GraphicsHardwareInterface_VK::FlushCommands(bool waitExecution, bool flushImplicitCommands)
	{
		uint32_t cmdBufferSubmitMask = (uint32_t)ECommandBufferUsageFlagBits_VK::Explicit;
		if (flushImplicitCommands)
		{
			cmdBufferSubmitMask |= (uint32_t)ECommandBufferUsageFlagBits_VK::Implicit;
		}

		auto pSemaphore = m_pMainDevice->pSyncObjectManager->RequestTimelineSemaphore();
		m_pMainDevice->pGraphicsCommandManager->SubmitCommandBuffers(pSemaphore, cmdBufferSubmitMask);

		if (flushImplicitCommands)
		{
			m_pMainDevice->pImplicitCmdBuffer = m_pMainDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
		}

		if (waitExecution)
		{
			if (pSemaphore->Wait(FRAME_TIMEOUT) != VK_SUCCESS)
			{
				throw std::runtime_error("Vulkan: Flush command timeout.");
			}
		}
	}

	void GraphicsHardwareInterface_VK::FlushTransferCommands(bool waitExecution)
	{
		if (m_pMainDevice->pTransferCommandManager == nullptr)
		{
			LOG_ERROR("Vulkan: Shouldn't flush transfer commands when transfer queue is not supported.");
			return;
		}

		uint32_t cmdBufferSubmitMask = (uint32_t)ECommandBufferUsageFlagBits_VK::Explicit | (uint32_t)ECommandBufferUsageFlagBits_VK::Implicit;

		auto pSemaphore = m_pMainDevice->pSyncObjectManager->RequestTimelineSemaphore();
		m_pMainDevice->pTransferCommandManager->SubmitCommandBuffers(pSemaphore, cmdBufferSubmitMask);

		if (waitExecution)
		{
			pSemaphore->Wait();
		}
	}

	void GraphicsHardwareInterface_VK::WaitSemaphore(GraphicsSemaphore* pSemaphore)
	{
		auto pVkSemaphore = (TimelineSemaphore_VK*)pSemaphore;
		pVkSemaphore->Wait(FRAME_TIMEOUT);
		m_pMainDevice->pSyncObjectManager->ReturnTimelineSemaphore(pVkSemaphore);
	}

	void GraphicsHardwareInterface_VK::WaitIdle()
	{
		m_pMainDevice->pGraphicsCommandManager->WaitWorkingQueueIdle();
	}

	void GraphicsHardwareInterface_VK::GetSwapchainImages(std::vector<Texture2D*>& outImages) const
	{
		DEBUG_ASSERT_CE(m_pSwapchain != nullptr);

		uint32_t count = m_pSwapchain->GetSwapchainImageCount();
		for (uint32_t i = 0; i < count; i++)
		{
			outImages.emplace_back(m_pSwapchain->GetSwapchainImageByIndex(i));
		}
	}

	uint32_t GraphicsHardwareInterface_VK::GetSwapchainPresentImageIndex() const
	{
		DEBUG_ASSERT_CE(m_pSwapchain != nullptr);
		return m_pSwapchain->GetTargetImageIndex();
	}

	void GraphicsHardwareInterface_VK::Present(uint32_t frameIndex)
	{
		DEBUG_ASSERT_CE(m_pSwapchain != nullptr);

		// Present current frame

		uint32_t cmdBufferSubmitMask = (uint32_t)ECommandBufferUsageFlagBits_VK::Explicit | (uint32_t)ECommandBufferUsageFlagBits_VK::Implicit;

		auto pRenderFinishSemaphore = m_pMainDevice->pSyncObjectManager->RequestSemaphore();
		m_pMainDevice->pImplicitCmdBuffer->SignalPresentationSemaphore(pRenderFinishSemaphore);
		m_renderFinishSemaphores.push(pRenderFinishSemaphore);

		auto pFrameSemaphore = m_pMainDevice->pSyncObjectManager->RequestTimelineSemaphore();
		m_pMainDevice->pGraphicsCommandManager->SubmitCommandBuffers(pFrameSemaphore, cmdBufferSubmitMask, &m_commandSubmissionSemaphore);
		m_frameSemaphores.push(pFrameSemaphore);

		if (m_frameSemaphores.size() >= m_pSwapchain->GetMaxFramesInFlight())
		{
			if (m_frameSemaphores.front()->Wait(FRAME_TIMEOUT) != VK_SUCCESS)
			{
				throw std::runtime_error("Vulkan: Frame timeline semaphore timeout.");
			}
			m_frameSemaphores.pop();

			m_pMainDevice->pSyncObjectManager->ReturnSemaphore(m_renderFinishSemaphores.front());
			m_renderFinishSemaphores.pop();
			DEBUG_ASSERT_CE(m_frameSemaphores.size() == m_renderFinishSemaphores.size());
		}

		// Becasue command buffers are submitted on a separate thread, we need to make sure pRenderFinishSemaphore
		// is submitted before we present the frame
		m_commandSubmissionSemaphore.Wait();

		std::vector<Semaphore_VK*> presentWaitSemaphores = { pRenderFinishSemaphore };
		bool presentSuccess = m_pSwapchain->Present(presentWaitSemaphores);

		// Preparation for next frame

		m_pMainDevice->pImplicitCmdBuffer = m_pMainDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();

		if (presentSuccess)
		{
			m_pSwapchain->UpdateBackBuffer(frameIndex);
			m_pMainDevice->pImplicitCmdBuffer->WaitPresentationSemaphore(m_pSwapchain->GetImageAvailableSemaphore(frameIndex));
		}
	}

	void GraphicsHardwareInterface_VK::ResizeSwapchain(uint32_t width, uint32_t height)
	{
		DestroySwapchain();
		SetupSwapchain(width, height);
	}

	bool GraphicsHardwareInterface_VK::CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, DataTransferBuffer*& pOutput)
	{
		RawBufferCreateInfo_VK vkBufferCreateInfo{};
		vkBufferCreateInfo.size = createInfo.size;
		vkBufferCreateInfo.usage = VulkanDataTransferBufferUsage(createInfo.usageFlags);
		vkBufferCreateInfo.memoryUsage = VulkanMemoryUsage(createInfo.memoryType);

		auto pDevice = m_pMainDevice;

		CE_NEW(pOutput, DataTransferBuffer_VK, pDevice->pUploadAllocator, vkBufferCreateInfo);

		if (createInfo.cpuMapped)
		{
			auto pVkBuffer = (DataTransferBuffer_VK*)pOutput;
			pDevice->pUploadAllocator->MapMemory(pVkBuffer->m_pBufferImpl->m_allocation, &pVkBuffer->m_ppMappedData);
			pVkBuffer->m_constantlyMapped = true;
		}

		return true;
	}

	void GraphicsHardwareInterface_VK::CopyTexture2DToDataTransferBuffer(Texture2D* pSrcTexture, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer)
	{
		auto pVkTexture = (Texture2D_VK*)pSrcTexture;
		auto pVkBuffer = (DataTransferBuffer_VK*)pDstBuffer;

		DEBUG_ASSERT_CE(pVkTexture->m_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL); // Or manual transition can be performed here
		DEBUG_ASSERT_CE(pVkBuffer->m_sizeInBytes >= pVkTexture->m_width * pVkTexture->m_height * VulkanFormatUnitSize(pVkTexture->m_format));

		std::vector<VkBufferImageCopy> copyRegions;
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;  // Tightly packed
		region.bufferImageHeight = 0;// Tightly packed
		region.imageSubresource.aspectMask = (pVkTexture->GetTextureType() == ETextureType::DepthAttachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { pVkTexture->m_width, pVkTexture->m_height, 1 };
		copyRegions.emplace_back(region);

		((CommandBuffer_VK*)pCommandBuffer)->CopyTexture2DToBuffer(pVkTexture, pVkBuffer->m_pBufferImpl, copyRegions);
	}

	void GraphicsHardwareInterface_VK::CopyDataTransferBufferToTexture2D(DataTransferBuffer* pSrcBuffer, Texture2D* pDstTexture, GraphicsCommandBuffer* pCommandBuffer)
	{
		auto pVkTexture = (Texture2D_VK*)pDstTexture;
		auto pVkBuffer = (DataTransferBuffer_VK*)pSrcBuffer;
		auto pVkCmdBuffer = (CommandBuffer_VK*)pCommandBuffer;

		DEBUG_ASSERT_CE(pVkBuffer->m_sizeInBytes <= pVkTexture->m_width * pVkTexture->m_height * VulkanFormatUnitSize(pVkTexture->m_format));

		std::vector<VkBufferImageCopy> copyRegions;
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;  // Tightly packed
		region.bufferImageHeight = 0;// Tightly packed
		region.imageSubresource.aspectMask = (pVkTexture->GetTextureType() == ETextureType::DepthAttachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { pVkTexture->m_width, pVkTexture->m_height, 1 };
		copyRegions.emplace_back(region);

		VkImageLayout originalLayout = pVkTexture->m_layout;
		uint32_t originalAppliedStages = pVkTexture->m_appliedStages;

		pVkCmdBuffer->TransitionImageLayout(pVkTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);
		pVkCmdBuffer->CopyBufferToTexture2D(pVkBuffer->m_pBufferImpl, pVkTexture, copyRegions);
		pVkCmdBuffer->TransitionImageLayout(pVkTexture, originalLayout, originalAppliedStages);
	}

	void GraphicsHardwareInterface_VK::CopyHostDataToDataTransferBuffer(void* pData, DataTransferBuffer* pDstBuffer, size_t size)
	{
		auto pDstVkBuffer = (DataTransferBuffer_VK*)pDstBuffer;

		DEBUG_ASSERT_CE(pDstVkBuffer->m_sizeInBytes >= size);

		if (!pDstVkBuffer->m_constantlyMapped)
		{
			pDstVkBuffer->m_pAllocator->MapMemory(pDstVkBuffer->m_pBufferImpl->m_allocation, &pDstVkBuffer->m_ppMappedData);
		}

		memcpy(pDstVkBuffer->m_ppMappedData, pData, size);

		if (!pDstVkBuffer->m_constantlyMapped)
		{
			pDstVkBuffer->m_pAllocator->UnmapMemory(pDstVkBuffer->m_pBufferImpl->m_allocation);
		}
	}

	void GraphicsHardwareInterface_VK::CopyDataTransferBufferToHostDataLocation(DataTransferBuffer* pSrcBuffer, void* pDataLoc)
	{
		auto pSrcVkBuffer = (DataTransferBuffer_VK*)pSrcBuffer;

		if (!pSrcVkBuffer->m_constantlyMapped)
		{
			pSrcVkBuffer->m_pAllocator->MapMemory(pSrcVkBuffer->m_pBufferImpl->m_allocation, &pSrcVkBuffer->m_ppMappedData);
		}

		memcpy(pDataLoc, pSrcVkBuffer->m_ppMappedData, pSrcVkBuffer->m_sizeInBytes);

		if (!pSrcVkBuffer->m_constantlyMapped)
		{
			pSrcVkBuffer->m_pAllocator->UnmapMemory(pSrcVkBuffer->m_pBufferImpl->m_allocation);
		}
	}

	void GraphicsHardwareInterface_VK::SetupDevice()
	{
		GetRequiredExtensions();
		CreateInstance();
		SetupDebugMessenger();
		CreatePresentationSurface();
		SelectPhysicalDevice();
		CreateLogicalDevice();
	}

	LogicalDevice_VK* GraphicsHardwareInterface_VK::GetLogicalDevice() const
	{
		return m_pMainDevice;
	}

	void GraphicsHardwareInterface_VK::GetRequiredExtensions()
	{
		m_requiredInstanceExtensions = GetRequiredInstanceExtensions_VK(m_enableValidationLayers);
		m_requiredDeviceExtensions = GetRequiredDeviceExtensions_VK();
		m_validationLayers = GetValidationLayerNames_VK();
	}

	void GraphicsHardwareInterface_VK::CreateInstance()
	{
		// Check validation layers
		if (m_enableValidationLayers && !CheckValidationLayerSupport_VK(m_validationLayers))
		{
			throw std::runtime_error("Vulkan: Validation layers requested, but not available.");
		}

		// Check extentions support
		if (!CheckAvailableInstanceExtensions_VK(m_availableExtensions))
		{
			return;
		}
		for (auto& extension : m_requiredInstanceExtensions)
		{
			if (!IsExtensionSupported_VK(m_availableExtensions, extension))
			{
				LOG_MESSAGE((std::string)"Vulkan: Extension '" + extension + "' is not supported.");
				return;
			}
		}

		// Generate application info
		m_appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		m_appInfo.pApplicationName = gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->GetAppName();
		m_appInfo.applicationVersion = 1;
		m_appInfo.pEngineName = "Cactus Engine";
		m_appInfo.engineVersion = 1;
		m_appInfo.apiVersion = VULKAN_VERSION_CE;

		// Generate creation info
		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &m_appInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredInstanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = m_requiredInstanceExtensions.size() > 0 ? m_requiredInstanceExtensions.data() : nullptr;

		// Configure debug messenger
		if (m_enableValidationLayers)
		{
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
			auto debugUtilsMessengerCreateInfo = GetDebugUtilsMessengerCreateInfo_VK();
			instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
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

		// Load all required Vulkan entrypoints, including all extensions
		volkLoadInstance(m_instance);
	}

	void GraphicsHardwareInterface_VK::SetupDebugMessenger()
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

	void GraphicsHardwareInterface_VK::CreatePresentationSurface()
	{
	#if defined(VK_USE_PLATFORM_WIN32_KHR)

	#if defined(GLFW_IMPLEMENTATION_CE)
		auto glfwWindow = reinterpret_cast<GLFWwindow*>(gpGlobal->GetWindowHandle());
		VkResult result = glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &m_presentationSurface);
	#else
		VkWin32SurfaceCreateInfoKHR surfaceCreationInfo{};
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
			throw std::runtime_error("Vulkan: Failed to create presentation surface.");
		}
	}

	void GraphicsHardwareInterface_VK::SelectPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		VkResult result;

		result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0 || (result != VK_SUCCESS))
		{
			throw std::runtime_error("Vulkan: Failed to find a GPU with Vulkan support.");
			return;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		std::vector<VkPhysicalDevice> suitableDevices;
		for (const auto& device : devices)
		{
			if (IsPhysicalDeviceSuitable_VK(device, m_presentationSurface, m_requiredDeviceExtensions))
			{
				suitableDevices.emplace_back(device);
			}
		}

		if (suitableDevices.size() == 0)
		{
			throw std::runtime_error("Vulkan: Failed to find a suitable GPU.");
			return;
		}

		CE_NEW(m_pMainDevice, LogicalDevice_VK);
		VkPhysicalDeviceType preferredGPUType = VulkanPhysicalDeviceType(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetPreferredGPUType());

		for (const auto& device : suitableDevices)
		{
			VkPhysicalDeviceProperties deviceProperties{};
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			if (deviceProperties.deviceType == preferredGPUType)
			{
				m_pMainDevice->physicalDevice = device;
				m_pMainDevice->deviceProperties = deviceProperties;

	#if defined(DEBUG_MODE_CE)
				DEBUG_LOG_MESSAGE("Found a suitable GPU:");
				PrintPhysicalDeviceInfo_VK(deviceProperties);
	#endif
				return;
			}
		}

		// Fallback to non-preferred GPU type
		{
			const VkPhysicalDevice& fallbackDevice = suitableDevices[0];

			VkPhysicalDeviceProperties deviceProperties{};
			vkGetPhysicalDeviceProperties(fallbackDevice, &deviceProperties);

			m_pMainDevice->physicalDevice = fallbackDevice;
			m_pMainDevice->deviceProperties = deviceProperties;

			LOG_MESSAGE((std::string)"Vulkan: Preferred GPU type is not supported, falling back to " + deviceProperties.deviceName);

	#if defined(DEBUG_MODE_CE)
			PrintPhysicalDeviceInfo_VK(deviceProperties);
	#endif
		}
	}

	void GraphicsHardwareInterface_VK::CreateLogicalDevice()
	{
		QueueFamilyIndices_VK queueFamilyIndices = FindQueueFamilies_VK(m_pMainDevice->physicalDevice, m_presentationSurface);

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
		if (queueFamilyIndices.transferFamily.has_value())
		{
			uniqueQueueFamilies.emplace(queueFamilyIndices.transferFamily.value());
		}

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Enabling timeline semaphore
		VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
		timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineSemaphoreFeatures.timelineSemaphore = true;

#if defined(VK_KHR_maintenance4)
		VkPhysicalDeviceMaintenance4Features maintenance4Feature{};
		maintenance4Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
		maintenance4Feature.maintenance4 = true;

		timelineSemaphoreFeatures.pNext = &maintenance4Feature;
#endif

		// TODO: configure device features by configuration settings
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.pNext = &timelineSemaphoreFeatures;
		physicalDeviceFeatures2.features = deviceFeatures;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &physicalDeviceFeatures2;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_requiredDeviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = m_requiredDeviceExtensions.data();

		if (m_enableValidationLayers)
		{
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
		}
		else
		{
			deviceCreateInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(m_pMainDevice->physicalDevice, &deviceCreateInfo, nullptr, &m_pMainDevice->logicalDevice);

		if (result != VK_SUCCESS || m_pMainDevice->logicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan: Failed to create logical device.");
			return;
		}

		// Load device-related Vulkan entrypoints directly from the driver
		volkLoadDevice(m_pMainDevice->logicalDevice);

		if (queueFamilyIndices.graphicsFamily.has_value())
		{
			m_pMainDevice->graphicsQueue.type = EQueueType::Graphics;
			m_pMainDevice->graphicsQueue.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
			vkGetDeviceQueue(m_pMainDevice->logicalDevice, m_pMainDevice->graphicsQueue.queueFamilyIndex, 0, &m_pMainDevice->graphicsQueue.queue);
		}

		if (queueFamilyIndices.presentFamily.has_value())
		{
			m_pMainDevice->presentQueue.type = EQueueType::Present;
			m_pMainDevice->presentQueue.queueFamilyIndex = queueFamilyIndices.presentFamily.value();
			vkGetDeviceQueue(m_pMainDevice->logicalDevice, m_pMainDevice->presentQueue.queueFamilyIndex, 0, &m_pMainDevice->presentQueue.queue);
		}

		// Query timeline semaphore function support
		//std::cout << vkGetDeviceProcAddr(pDevice->logicalDevice, "vkWaitSemaphores") << std::endl;

		if (queueFamilyIndices.transferFamily.has_value())
		{
			m_pMainDevice->transferQueue.type = EQueueType::Transfer;
			m_pMainDevice->transferQueue.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
			vkGetDeviceQueue(m_pMainDevice->logicalDevice, m_pMainDevice->transferQueue.queueFamilyIndex, 0, &m_pMainDevice->transferQueue.queue);
		}
		else
		{
			m_pMainDevice->transferQueue.isValid = false;
		}

		DEBUG_LOG_MESSAGE((std::string)"Vulkan: Logical device created on " + m_pMainDevice->deviceProperties.deviceName);
	}

	void GraphicsHardwareInterface_VK::SetupSwapchain()
	{
		SetupSwapchain(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight());
	}

	void GraphicsHardwareInterface_VK::SetupSwapchain(uint32_t width, uint32_t height)
	{
		SwapchainCreateInfo_VK createInfo{};
		createInfo.maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		createInfo.queueFamilyIndices = FindQueueFamilies_VK(m_pMainDevice->physicalDevice, m_presentationSurface);
		createInfo.supportDetails = QuerySwapchainSupport_VK(m_pMainDevice->physicalDevice, m_presentationSurface);
		createInfo.surface = m_presentationSurface;
		createInfo.surfaceFormat = ChooseSwapSurfaceFormat(createInfo.supportDetails.formats);
		createInfo.swapExtent = { width, height };

		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetVSync())
		{
			for (const auto& availablePresentMode : createInfo.supportDetails.presentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					break;
				}
			}
		}

		CE_NEW(m_pSwapchain, Swapchain_VK, m_pMainDevice, createInfo);

		m_pSwapchain->UpdateBackBuffer(0);
		m_pMainDevice->pImplicitCmdBuffer->WaitPresentationSemaphore(m_pSwapchain->GetImageAvailableSemaphore(0));
	}

	void GraphicsHardwareInterface_VK::DestroySwapchain()
	{
		CE_DELETE(m_pSwapchain);
	}

	VkSurfaceFormatKHR GraphicsHardwareInterface_VK::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& availableFormat : availableFormats)
		{
			// Alert: this might not be universal
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		LOG_ERROR("Vulkan: No suitable swap surface format is found.");
		return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	void GraphicsHardwareInterface_VK::CreateShaderModuleFromFile(const char* shaderFilePath, LogicalDevice_VK* pLogicalDevice, VkShaderModule& outModule, std::vector<char>& outRawCode)
	{
		std::ifstream file(shaderFilePath, std::ios::ate | std::ios::binary | std::ios::in);

		if (!file.is_open())
		{
			throw std::runtime_error("Vulkan: failed to open " + *shaderFilePath);
			return;
		}

		size_t fileSize = (size_t)file.tellg();
		DEBUG_ASSERT_CE(fileSize > 0);
		outRawCode.resize(fileSize);

		file.seekg(0, std::ios::beg);
		file.read(outRawCode.data(), fileSize);

		file.close();
		DEBUG_ASSERT_CE(outRawCode.size() > 0);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = fileSize;
		createInfo.pCode = reinterpret_cast<uint32_t*>(outRawCode.data());

		VkResult result = vkCreateShaderModule(pLogicalDevice->logicalDevice, &createInfo, nullptr, &outModule);
		DEBUG_ASSERT_CE(outModule != VK_NULL_HANDLE && result == VK_SUCCESS);
	}

	void GraphicsHardwareInterface_VK::SetupCommandManager()
	{
		// Graphics queue by default must be supported
		CE_NEW(m_pMainDevice->pGraphicsCommandManager, CommandManager_VK, m_pMainDevice, m_pMainDevice->graphicsQueue);

		if (m_pMainDevice->transferQueue.isValid)
		{
			CE_NEW(m_pMainDevice->pTransferCommandManager, CommandManager_VK, m_pMainDevice, m_pMainDevice->transferQueue);
		}
		else
		{
			m_pMainDevice->pTransferCommandManager = nullptr;
		}

		m_pMainDevice->pImplicitCmdBuffer = m_pMainDevice->pGraphicsCommandManager->RequestPrimaryCommandBuffer();
	}

	void GraphicsHardwareInterface_VK::SetupSyncObjectManager()
	{
		CE_NEW(m_pMainDevice->pSyncObjectManager, SyncObjectManager_VK, m_pMainDevice);
	}

	void GraphicsHardwareInterface_VK::SetupUploadAllocator()
	{
		CE_NEW(m_pMainDevice->pUploadAllocator, UploadAllocator_VK, m_pMainDevice, m_instance);
	}

	void GraphicsHardwareInterface_VK::SetupDescriptorAllocator()
	{
		CE_NEW(m_pMainDevice->pDescriptorAllocator, DescriptorAllocator_VK, m_pMainDevice);
	}

	EDescriptorResourceType_VK GraphicsHardwareInterface_VK::VulkanDescriptorResourceType(EDescriptorType type) const
	{
		switch (type)
		{
		case EDescriptorType::UniformBuffer:
		case EDescriptorType::StorageBuffer:
		case EDescriptorType::SubUniformBuffer:
			return EDescriptorResourceType_VK::Buffer;

		case EDescriptorType::SampledImage:
		case EDescriptorType::CombinedImageSampler:
		case EDescriptorType::Sampler: // Alert: could be wrong to classify as image
		case EDescriptorType::StorageImage:
			return EDescriptorResourceType_VK::Image;

		case EDescriptorType::UniformTexelBuffer:
		case EDescriptorType::StorageTexelBuffer:
			return EDescriptorResourceType_VK::TexelBuffer;

		default:
			LOG_ERROR("Vulkan: Unhandled descriptor type: " + std::to_string((uint32_t)type));
			return EDescriptorResourceType_VK::Buffer;
		}
	}

	void GraphicsHardwareInterface_VK::GetBufferInfoByDescriptorType(EDescriptorType type, const RawResource* pRes, VkDescriptorBufferInfo& outInfo)
	{
		switch (type)
		{
		case EDescriptorType::UniformBuffer:
		{
			auto pBuffer = (UniformBuffer_VK*)pRes;
			outInfo.buffer = pBuffer->GetBufferImpl()->m_buffer;
			outInfo.offset = 0;
			outInfo.range = VK_WHOLE_SIZE;
			break;
		}
		case EDescriptorType::SubUniformBuffer:
		{
			auto pBuffer = (SubUniformBuffer*)pRes;
			outInfo.buffer = ((UniformBuffer_VK*)(pBuffer->m_pParentBuffer))->GetBufferImpl()->m_buffer;
			outInfo.offset = pBuffer->m_offset;
			outInfo.range = pBuffer->GetSizeInBytes();
			break;
		}
		default:
			throw std::runtime_error("Vulkan: Unhandled buffer descriptor type or misclassified buffer descriptor type.");
		}
	}
}