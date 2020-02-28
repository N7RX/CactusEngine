#include "HPForwardRenderer.h"
#include "DrawingSystem.h"
#include "AllComponents.h"
#include "Timer.h"
#include "ImageTexture.h"

// For line drawing computation
#include <Eigen/Dense>

#include <iostream>

using namespace Engine;

HPForwardRenderer::HPForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(ERendererType::Forward, pDevice, pSystem)
{
}

void HPForwardRenderer::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>();

	BuildRenderResources();

	BuildShadowMapPass();
	BuildNormalOnlyPass();
	BuildOpaquePass();
	BuildGaussianBlurPass();
	BuildLineDrawingPass();
	BuildTransparentPass();
	BuildOpaqueTranspBlendPass();
	BuildDepthOfFieldPass();
}

void HPForwardRenderer::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pContext = std::make_shared<RenderContext>();
	pContext->pCamera = pCamera;
	pContext->pDrawList = &drawList;
	pContext->pRenderer = this;

	m_pRenderGraph->BeginRenderPasses(pContext);
}

const std::shared_ptr<GraphicsPipelineObject> HPForwardRenderer::GetGraphicsPipeline(EBuiltInShaderProgramType shaderType, const char* passName) const
{
	if (m_graphicsPipelines.find(shaderType) != m_graphicsPipelines.end())
	{
		auto pPipelines = m_graphicsPipelines.at(shaderType);

		if (pPipelines.find(passName) != pPipelines.end())
		{
			return pPipelines.at(passName);
		}
		else
		{
			std::cerr << "Couldn't find associated graphics pipeline with given render pass.\n";
		}
	}
	else
	{
		std::cerr << "Couldn't find associated graphics pipelines with given shader type.\n";
	}
	return nullptr;
}

void HPForwardRenderer::BuildRenderResources()
{
	CreateRenderPassObjects();
	CreateFrameResources();
	TransitionResourceLayouts();
	CreatePipelineObjects();
}

void HPForwardRenderer::CreateFrameResources()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.generateMipmap = false;
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete); // TODO: assign sampler for integrated device as well

	// Depth attachment for shadow map pass

	texCreateInfo.textureWidth = 4096; // Shadow map resolution
	texCreateInfo.textureHeight = 4096;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pShadowMapPassDepthOutput);

	// Frame buffer for shadow map pass

	FrameBufferCreateInfo shadowMapFBCreateInfo = {};
	shadowMapFBCreateInfo.attachments.emplace_back(m_pShadowMapPassDepthOutput);
	shadowMapFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	shadowMapFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;
	shadowMapFBCreateInfo.deviceType = EGPUType::Discrete;
	shadowMapFBCreateInfo.pRenderPass = m_pShadowMapRenderPassObject;

	m_pDevice->CreateFrameBuffer(shadowMapFBCreateInfo, m_pShadowMapPassFrameBuffer);

	// Color outputs from normal-only pass

	texCreateInfo.textureWidth = screenWidth;
	texCreateInfo.textureHeight = screenHeight;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassNormalOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassPositionOutput);

	// Depth attachment for normal-only pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassDepthOutput);

	// Frame buffer for normal-only pass

	FrameBufferCreateInfo normalOnlyFBCreateInfo = {};
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassNormalOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassPositionOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassDepthOutput);
	normalOnlyFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	normalOnlyFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;
	normalOnlyFBCreateInfo.deviceType = EGPUType::Discrete;
	normalOnlyFBCreateInfo.pRenderPass = m_pNormalOnlyRenderPassObject;

	m_pDevice->CreateFrameBuffer(normalOnlyFBCreateInfo, m_pNormalOnlyPassFrameBuffer);

	// Color output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassColorOutput);

	// Shadow color output from opaque pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassShadowOutput);

	// Depth output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassDepthOutput);

	// Frame buffer for opaque pass

	FrameBufferCreateInfo opaqueFBCreateInfo = {};
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassColorOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassShadowOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassDepthOutput);
	opaqueFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	opaqueFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;
	opaqueFBCreateInfo.deviceType = EGPUType::Discrete;
	opaqueFBCreateInfo.pRenderPass = m_pOpaqueRenderPassObject;

	m_pDevice->CreateFrameBuffer(opaqueFBCreateInfo, m_pOpaquePassFrameBuffer);

	// Color outputs from Gaussian blur pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassHorizontalColorOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassFinalColorOutput);

	// Frame buffers for Gaussian blur pass

	FrameBufferCreateInfo blurFBCreateInfo_Horizontal = {};
	blurFBCreateInfo_Horizontal.attachments.emplace_back(m_pBlurPassHorizontalColorOutput);
	blurFBCreateInfo_Horizontal.framebufferWidth = texCreateInfo.textureWidth;
	blurFBCreateInfo_Horizontal.framebufferHeight = texCreateInfo.textureHeight;
	blurFBCreateInfo_Horizontal.deviceType = EGPUType::Discrete;
	blurFBCreateInfo_Horizontal.pRenderPass = m_pBlurRenderPassObject; // Also compatible with line drawing pass

	FrameBufferCreateInfo blurFBCreateInfo_FinalColor = {};
	blurFBCreateInfo_FinalColor.attachments.emplace_back(m_pBlurPassFinalColorOutput);
	blurFBCreateInfo_FinalColor.framebufferWidth = texCreateInfo.textureWidth;
	blurFBCreateInfo_FinalColor.framebufferHeight = texCreateInfo.textureHeight;
	blurFBCreateInfo_FinalColor.deviceType = EGPUType::Discrete;
	blurFBCreateInfo_FinalColor.pRenderPass = m_pBlurRenderPassObject;

	m_pDevice->CreateFrameBuffer(blurFBCreateInfo_Horizontal, m_pBlurPassFrameBuffer_Horizontal);
	m_pDevice->CreateFrameBuffer(blurFBCreateInfo_Horizontal, m_pBlurPassFrameBuffer_FinalColor);

	// Curvature output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassCurvatureOutput);

	// Blurred output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassBlurredOutput);

	// Color output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassColorOutput);

	// Frame buffers for line drawing pass

	FrameBufferCreateInfo lineDrawFBCreateInfo_Curvature = {};
	lineDrawFBCreateInfo_Curvature.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo_Curvature.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo_Curvature.framebufferHeight = texCreateInfo.textureHeight;
	lineDrawFBCreateInfo_Curvature.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Curvature.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_RidgeSearch = {};
	lineDrawFBCreateInfo_RidgeSearch.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo_RidgeSearch.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo_RidgeSearch.framebufferHeight = texCreateInfo.textureHeight;
	lineDrawFBCreateInfo_RidgeSearch.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_RidgeSearch.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_Blur_Horizontal = {};
	lineDrawFBCreateInfo_Blur_Horizontal.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo_Blur_Horizontal.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo_Blur_Horizontal.framebufferHeight = texCreateInfo.textureHeight;
	lineDrawFBCreateInfo_Blur_Horizontal.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Blur_Horizontal.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_Blur_FinalColor = {};
	lineDrawFBCreateInfo_Blur_FinalColor.attachments.emplace_back(m_pLineDrawingPassBlurredOutput);
	lineDrawFBCreateInfo_Blur_FinalColor.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo_Blur_FinalColor.framebufferHeight = texCreateInfo.textureHeight;
	lineDrawFBCreateInfo_Blur_FinalColor.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Blur_FinalColor.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_FinalBlend = {};
	lineDrawFBCreateInfo_FinalBlend.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo_FinalBlend.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo_FinalBlend.framebufferHeight = texCreateInfo.textureHeight;
	lineDrawFBCreateInfo_FinalBlend.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_FinalBlend.pRenderPass = m_pLineDrawingRenderPassObject;

	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Curvature, m_pLineDrawingPassFrameBuffer_Curvature);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_RidgeSearch, m_pLineDrawingPassFrameBuffer_RidgeSearch);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Blur_Horizontal, m_pLineDrawingPassFrameBuffer_Blur_Horizontal);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Blur_FinalColor, m_pLineDrawingPassFrameBuffer_Blur_FinalColor);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_FinalBlend, m_pLineDrawingPassFrameBuffer_FinalBlend);

	// Color output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassColorOutput);

	// Depth output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassDepthOutput);

	// Frame buffer for transparent pass

	FrameBufferCreateInfo transpFBCreateInfo = {};
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassColorOutput);
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassDepthOutput);
	transpFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	transpFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;
	transpFBCreateInfo.deviceType = EGPUType::Discrete;
	transpFBCreateInfo.pRenderPass = m_pTranspRenderPassObject;

	m_pDevice->CreateFrameBuffer(transpFBCreateInfo, m_pTranspPassFrameBuffer);

	// Color output from blend pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlendPassColorOutput);

	// Frame buffer for blend pass

	FrameBufferCreateInfo blendFBCreateInfo = {};
	blendFBCreateInfo.attachments.emplace_back(m_pBlendPassColorOutput);
	blendFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	blendFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;
	blendFBCreateInfo.deviceType = EGPUType::Discrete;
	blendFBCreateInfo.pRenderPass = m_pBlendRenderPassObject;

	m_pDevice->CreateFrameBuffer(blendFBCreateInfo, m_pBlendPassFrameBuffer);

	// Horizontal color output from DOF pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pDOFPassHorizontalOutput);

	// Frame buffer for DOF pass

	FrameBufferCreateInfo dofFBCreateInfo_Horizontal = {};
	dofFBCreateInfo_Horizontal.attachments.emplace_back(m_pDOFPassHorizontalOutput);
	dofFBCreateInfo_Horizontal.framebufferWidth = texCreateInfo.textureWidth;
	dofFBCreateInfo_Horizontal.framebufferHeight = texCreateInfo.textureHeight;
	dofFBCreateInfo_Horizontal.deviceType = EGPUType::Discrete;
	dofFBCreateInfo_Horizontal.pRenderPass = m_pDOFRenderPassObject;

	m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Horizontal, m_pDOFPassFrameBuffer_Horizontal);

	// Create a framebuffer arround each swapchain image

	m_pDOFPassPresentFrameBuffers = std::make_shared<SwapchainFrameBuffers>();

	std::vector<std::shared_ptr<Texture2D>> swapchainImages;
	m_pDevice->GetSwapchainImages(swapchainImages);

	for (unsigned int i = 0; i < swapchainImages.size(); i++)
	{
		FrameBufferCreateInfo dofFBCreateInfo_Final = {};
		dofFBCreateInfo_Final.attachments.emplace_back(swapchainImages[i]);
		dofFBCreateInfo_Final.framebufferWidth = texCreateInfo.textureWidth;
		dofFBCreateInfo_Final.framebufferHeight = texCreateInfo.textureHeight;
		dofFBCreateInfo_Final.deviceType = EGPUType::Discrete;
		dofFBCreateInfo_Final.pRenderPass = m_pDOFRenderPassObject;

		std::shared_ptr<FrameBuffer> pFrameBuffer;
		m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Final, pFrameBuffer);
		m_pDOFPassPresentFrameBuffers->frameBuffers.emplace_back(pFrameBuffer);
	}

	// Post effects resources
	m_pBrushMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_1.png");
	m_pBrushMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_2.png");
	m_pPencilMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_1.jpg");
	m_pPencilMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_2.jpg");

	m_pBrushMaskImageTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pBrushMaskImageTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pPencilMaskImageTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pPencilMaskImageTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));

	// Uniform buffers
	UniformBufferCreateInfo ubCreateInfo = {};
	ubCreateInfo.deviceType = EGPUType::Discrete;

	ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex & (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex & (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pLightSpaceTransformMatrix_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pMaterialNumericalProperties_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraPropertie_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex & (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBControlVariables);
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pControlVariables_UB);
}

void HPForwardRenderer::TransitionResourceLayouts()
{
	m_pDevice->TransitionImageLayout(m_pShadowMapPassDepthOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pNormalOnlyPassNormalOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pNormalOnlyPassPositionOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pNormalOnlyPassDepthOutput, EImageLayout::DepthStencilAttachment, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pOpaquePassColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pOpaquePassShadowOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pOpaquePassDepthOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pBlurPassHorizontalColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pBlurPassFinalColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pLineDrawingPassCurvatureOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pLineDrawingPassBlurredOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pLineDrawingPassColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pLineDrawingMatrixTexture, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pTranspPassColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pTranspPassDepthOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pBlendPassColorOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pDOFPassHorizontalOutput, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->TransitionImageLayout(m_pBrushMaskImageTexture_1, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pBrushMaskImageTexture_2, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pPencilMaskImageTexture_1, EImageLayout::ShaderReadOnly, EShaderType::Fragment);
	m_pDevice->TransitionImageLayout(m_pPencilMaskImageTexture_2, EImageLayout::ShaderReadOnly, EShaderType::Fragment);

	m_pDevice->FlushCommands(true);
}

void HPForwardRenderer::CreateRenderPassObjects()
{
	// Shadow map pass
	{
		RenderPassCreateInfo shadowMapPassCreateInfo = {};
		shadowMapPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 0;

		shadowMapPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(shadowMapPassCreateInfo, m_pShadowMapRenderPassObject);
	}

	// Normal only pass
	{
		RenderPassCreateInfo normalOnlyPassCreateInfo = {};
		normalOnlyPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription normalDesc = {};
		normalDesc.format = ETextureFormat::RGBA32F;
		normalDesc.sampleCount = 1;
		normalDesc.loadOp = EAttachmentOperation::Clear;
		normalDesc.storeOp = EAttachmentOperation::Store;
		normalDesc.stencilLoadOp = EAttachmentOperation::None;
		normalDesc.stencilStoreOp = EAttachmentOperation::None;
		normalDesc.initialLayout = EImageLayout::ShaderReadOnly;
		normalDesc.usageLayout = EImageLayout::ColorAttachment;
		normalDesc.finalLayout = EImageLayout::ShaderReadOnly;
		normalDesc.type = EAttachmentType::Color;
		normalDesc.index = 0;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(normalDesc);

		RenderPassAttachmentDescription positionDesc = {};
		positionDesc.format = ETextureFormat::RGBA32F;
		positionDesc.sampleCount = 1;
		positionDesc.loadOp = EAttachmentOperation::Clear;
		positionDesc.storeOp = EAttachmentOperation::Store;
		positionDesc.stencilLoadOp = EAttachmentOperation::None;
		positionDesc.stencilStoreOp = EAttachmentOperation::None;
		positionDesc.initialLayout = EImageLayout::ShaderReadOnly;
		positionDesc.usageLayout = EImageLayout::ColorAttachment;
		positionDesc.finalLayout = EImageLayout::ShaderReadOnly;
		positionDesc.type = EAttachmentType::Color;
		positionDesc.index = 1;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(positionDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 2;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(normalOnlyPassCreateInfo, m_pNormalOnlyRenderPassObject);
	}

	// Opaque pass
	{
		RenderPassCreateInfo opaquePassCreateInfo = {};
		opaquePassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		RenderPassAttachmentDescription shadowDesc = {};
		shadowDesc.format = ETextureFormat::RGBA32F;
		shadowDesc.sampleCount = 1;
		shadowDesc.loadOp = EAttachmentOperation::Clear;
		shadowDesc.storeOp = EAttachmentOperation::Store;
		shadowDesc.stencilLoadOp = EAttachmentOperation::None;
		shadowDesc.stencilStoreOp = EAttachmentOperation::None;
		shadowDesc.initialLayout = EImageLayout::ShaderReadOnly;
		shadowDesc.usageLayout = EImageLayout::ColorAttachment;
		shadowDesc.finalLayout = EImageLayout::ShaderReadOnly;
		shadowDesc.type = EAttachmentType::Color;
		shadowDesc.index = 1;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(shadowDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly; // Will be read in later passes
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 2;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(opaquePassCreateInfo, m_pOpaqueRenderPassObject);
	}

	// Blur pass
	{
		RenderPassCreateInfo blurPassCreateInfo = {};
		blurPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		blurPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(blurPassCreateInfo, m_pBlurRenderPassObject);
	}

	// Line drawing pass
	{
		RenderPassCreateInfo lineDrawingPassCreateInfo = {};
		lineDrawingPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		lineDrawingPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(lineDrawingPassCreateInfo, m_pLineDrawingRenderPassObject);
	}

	// Transparent pass
	{
		RenderPassCreateInfo transparentPassCreateInfo = {};
		transparentPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		transparentPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly; // Will be read in later passes
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 1;

		transparentPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(transparentPassCreateInfo, m_pTranspRenderPassObject);
	}

	// Blend pass
	{
		RenderPassCreateInfo blendPassCreateInfo = {};
		blendPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		blendPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(blendPassCreateInfo, m_pBlendRenderPassObject);
	}

	// DOF pass
	{
		RenderPassCreateInfo dofPassCreateInfo = {};
		dofPassCreateInfo.deviceType = EGPUType::Discrete;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		dofPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(dofPassCreateInfo, m_pDOFRenderPassObject);
	}
}

void HPForwardRenderer::CreatePipelineObjects()
{
	// TODO: break this long function down into individual pass creation functions

	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Shared states

	// Vertex input state

	VertexInputBindingDescription positionBindingDesc = {};
	positionBindingDesc.binding = DrawingDevice::ATTRIB_POSITION_LOCATION; // Alert: bindng and location are different
	positionBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	positionBindingDesc.inputRate = EVertexInputRate::PerVertex;
	VertexInputAttributeDescription positionAttributeDesc = {};
	positionAttributeDesc.binding = positionBindingDesc.binding;
	positionAttributeDesc.location = DrawingDevice::ATTRIB_POSITION_LOCATION;
	positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
	positionAttributeDesc.format = ETextureFormat::UNDEFINED;

	VertexInputBindingDescription normalBindingDesc = {};
	normalBindingDesc.binding = DrawingDevice::ATTRIB_NORMAL_LOCATION;
	normalBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	normalBindingDesc.inputRate = EVertexInputRate::PerVertex;
	VertexInputAttributeDescription normalAttributeDesc = {};
	normalAttributeDesc.binding = normalBindingDesc.binding;
	normalAttributeDesc.location = DrawingDevice::ATTRIB_NORMAL_LOCATION;
	normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
	normalAttributeDesc.format = ETextureFormat::UNDEFINED;

	VertexInputBindingDescription texcoordBindingDesc = {};
	texcoordBindingDesc.binding = DrawingDevice::ATTRIB_TEXCOORD_LOCATION;
	texcoordBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	texcoordBindingDesc.inputRate = EVertexInputRate::PerVertex;
	VertexInputAttributeDescription texcoordAttributeDesc = {};
	texcoordAttributeDesc.binding = texcoordBindingDesc.binding;
	texcoordAttributeDesc.location = DrawingDevice::ATTRIB_TEXCOORD_LOCATION;
	texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
	texcoordAttributeDesc.format = ETextureFormat::UNDEFINED;

	VertexInputBindingDescription tangentBindingDesc = {};
	tangentBindingDesc.binding = DrawingDevice::ATTRIB_TANGENT_LOCATION;
	tangentBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	tangentBindingDesc.inputRate = EVertexInputRate::PerVertex;
	VertexInputAttributeDescription tangentAttributeDesc = {};
	tangentAttributeDesc.binding = tangentBindingDesc.binding;
	tangentAttributeDesc.location = DrawingDevice::ATTRIB_TANGENT_LOCATION;
	tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
	tangentAttributeDesc.format = ETextureFormat::UNDEFINED;

	VertexInputBindingDescription bitangentBindingDesc = {};
	bitangentBindingDesc.binding = DrawingDevice::ATTRIB_BITANGENT_LOCATION;
	bitangentBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	bitangentBindingDesc.inputRate = EVertexInputRate::PerVertex;
	VertexInputAttributeDescription bitangentAttributeDesc = {};
	bitangentAttributeDesc.binding = bitangentAttributeDesc.binding;
	bitangentAttributeDesc.location = DrawingDevice::ATTRIB_BITANGENT_LOCATION;
	bitangentAttributeDesc.offset = VertexBufferCreateInfo::bitangentOffset;
	bitangentAttributeDesc.format = ETextureFormat::UNDEFINED;

	PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.bindingDescs = { positionBindingDesc, normalBindingDesc, texcoordBindingDesc, tangentBindingDesc, bitangentBindingDesc };
	vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc, bitangentAttributeDesc };

	std::shared_ptr<PipelineVertexInputState> pVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, pVertexInputState);

	// For attributeless drawing

	PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
	emptyVertexInputStateCreateInfo.bindingDescs = {};
	emptyVertexInputStateCreateInfo.attributeDescs = {};

	std::shared_ptr<PipelineVertexInputState> pEmptyVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

	// Input assembly state

	PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
	inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

	std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState = nullptr;
	m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState);

	// Rasterization state
	
	PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
	rasterizationStateCreateInfo.enableDepthClamp = false;
	rasterizationStateCreateInfo.cullMode = ECullMode::Back;

	std::shared_ptr<PipelineRasterizationState> pRasterizationState = nullptr;
	m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pRasterizationState);

	// Depth stencil states

	PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.enableDepthTest = true;
	depthStencilStateCreateInfo.enableDepthWrite = true;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
	depthStencilStateCreateInfo.enableStencilTest = false;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState_DepthNoStencil = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState_DepthNoStencil);

	depthStencilStateCreateInfo.enableDepthTest = false;
	depthStencilStateCreateInfo.enableDepthWrite = false;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Always;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState_NoDepthNoStencil = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState_NoDepthNoStencil);

	// Multisample state

	PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.enableSampleShading = false;
	multisampleStateCreateInfo.sampleCount = 1;

	std::shared_ptr<PipelineMultisampleState> pMultisampleState = nullptr;
	m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, pMultisampleState);

	// Viewport state

	PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.width = screenWidth;
	viewportStateCreateInfo.height = screenHeight;

	std::shared_ptr<PipelineViewportState> pViewportState;
	m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

	// Individual states

	// With basic shader
	{
		GraphicsPipelineCreateInfo basicPipelineCreateInfo = {};
		basicPipelineCreateInfo.deviceType = EGPUType::Discrete;
		basicPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic);
		basicPipelineCreateInfo.pVertexInputState = pVertexInputState;
		basicPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		basicPipelineCreateInfo.pRasterizationState = pRasterizationState;
		basicPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		basicPipelineCreateInfo.pMultisampleState = pMultisampleState;
		basicPipelineCreateInfo.pViewportState = pViewportState;
		basicPipelineCreateInfo.pRenderPass = m_pOpaqueRenderPassObject; // Basic shader works under opaque pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(basicPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicPipelines;
		basicPipelines[HPForwardGraphRes::PASSNAME_OPAQUE] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::Basic] = basicPipelines;
	}

	// With basic transparent shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		AttachmentColorBlendStateDescription attachmentBlendDesc = {};
		attachmentBlendDesc.enableBlend = true;
		attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::OneMinusSrcAlpha;
		attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::OneMinusSrcAlpha;
		attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.alphaBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.pAttachment = m_pTranspPassColorOutput;
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo basicTranspPipelineCreateInfo = {};
		basicTranspPipelineCreateInfo.deviceType = EGPUType::Discrete;
		basicTranspPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic_Transparent);
		basicTranspPipelineCreateInfo.pVertexInputState = pVertexInputState;
		basicTranspPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		basicTranspPipelineCreateInfo.pColorBlendState = pColorBlendState;
		basicTranspPipelineCreateInfo.pRasterizationState = pRasterizationState;
		basicTranspPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		basicTranspPipelineCreateInfo.pMultisampleState = pMultisampleState;
		basicTranspPipelineCreateInfo.pViewportState = pViewportState;
		basicTranspPipelineCreateInfo.pRenderPass = m_pTranspRenderPassObject; // Basic transparent shader works under transparent pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(basicTranspPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicTranspPipelines;
		basicTranspPipelines[HPForwardGraphRes::PASSNAME_TRANSPARENT] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::Basic_Transparent] = basicTranspPipelines;
	}

	// With water basic shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		AttachmentColorBlendStateDescription attachmentBlendDesc = {};
		attachmentBlendDesc.enableBlend = true;
		attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::OneMinusSrcAlpha;
		attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::OneMinusSrcAlpha;
		attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.alphaBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.pAttachment = m_pTranspPassColorOutput;
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo waterBasicPipelineCreateInfo = {};
		waterBasicPipelineCreateInfo.deviceType = EGPUType::Discrete;
		waterBasicPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::WaterBasic);
		waterBasicPipelineCreateInfo.pVertexInputState = pVertexInputState;
		waterBasicPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		waterBasicPipelineCreateInfo.pColorBlendState = pColorBlendState;
		waterBasicPipelineCreateInfo.pRasterizationState = pRasterizationState;
		waterBasicPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		waterBasicPipelineCreateInfo.pMultisampleState = pMultisampleState;
		waterBasicPipelineCreateInfo.pViewportState = pViewportState;
		waterBasicPipelineCreateInfo.pRenderPass = m_pTranspRenderPassObject; // water basic shader works under transparent pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(waterBasicPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicWaterPipelines;
		basicWaterPipelines[HPForwardGraphRes::PASSNAME_TRANSPARENT] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::WaterBasic] = basicWaterPipelines;
	}

	// With depth based color blend shader
	{
		GraphicsPipelineCreateInfo blendPipelineCreateInfo = {};
		blendPipelineCreateInfo.deviceType = EGPUType::Discrete;
		blendPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		blendPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState; // Full screen quad dosen't have attribute input
		blendPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		blendPipelineCreateInfo.pRasterizationState = pRasterizationState;
		blendPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		blendPipelineCreateInfo.pMultisampleState = pMultisampleState;
		blendPipelineCreateInfo.pViewportState = pViewportState;
		blendPipelineCreateInfo.pRenderPass = m_pBlendRenderPassObject; // Depth based color blend shader works under blend pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(blendPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline blendPipelines;
		blendPipelines[HPForwardGraphRes::PASSNAME_BLEND] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::DepthBased_ColorBlend_2] = blendPipelines;
	}

	// With anime style shader
	{
		GraphicsPipelineCreateInfo animeStylePipelineCreateInfo = {};
		animeStylePipelineCreateInfo.deviceType = EGPUType::Discrete;
		animeStylePipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::AnimeStyle);
		animeStylePipelineCreateInfo.pVertexInputState = pVertexInputState;
		animeStylePipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		animeStylePipelineCreateInfo.pRasterizationState = pRasterizationState;
		animeStylePipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		animeStylePipelineCreateInfo.pMultisampleState = pMultisampleState;
		animeStylePipelineCreateInfo.pViewportState = pViewportState;
		animeStylePipelineCreateInfo.pRenderPass = m_pOpaqueRenderPassObject; // Anime style shader works under opaque pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(animeStylePipelineCreateInfo, pPipeline);

		PassGraphicsPipeline animeStylePipelines;
		animeStylePipelines[HPForwardGraphRes::PASSNAME_OPAQUE] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::AnimeStyle] = animeStylePipelines;
	}

	// With normal only shader
	{
		GraphicsPipelineCreateInfo normalOnlyPipelineCreateInfo = {};
		normalOnlyPipelineCreateInfo.deviceType = EGPUType::Discrete;
		normalOnlyPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);
		normalOnlyPipelineCreateInfo.pVertexInputState = pVertexInputState;
		normalOnlyPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		normalOnlyPipelineCreateInfo.pRasterizationState = pRasterizationState;
		normalOnlyPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		normalOnlyPipelineCreateInfo.pMultisampleState = pMultisampleState;
		normalOnlyPipelineCreateInfo.pViewportState = pViewportState;
		normalOnlyPipelineCreateInfo.pRenderPass = m_pNormalOnlyRenderPassObject; // Normal only shader works under normal only pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(normalOnlyPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline normalOnlyPipelines;
		normalOnlyPipelines[HPForwardGraphRes::PASSNAME_NORMAL_ONLY] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::NormalOnly] = normalOnlyPipelines;
	}

	// With line drawing curvature shader
	{
		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing curvature shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Curvature] = lineDrawingPipelines;
	}

	// With line drawing color shader
	{
		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing curvature shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Color] = lineDrawingPipelines;
	}

	// With line drawing blend shader
	{
		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing blend shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Blend] = lineDrawingPipelines;
	}

	// With Gaussian blur shader
	{
		GraphicsPipelineCreateInfo blurPipelineCreateInfo = {};
		blurPipelineCreateInfo.deviceType = EGPUType::Discrete;
		blurPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
		blurPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		blurPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		blurPipelineCreateInfo.pRasterizationState = pRasterizationState;
		blurPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		blurPipelineCreateInfo.pMultisampleState = pMultisampleState;
		blurPipelineCreateInfo.pViewportState = pViewportState;
		blurPipelineCreateInfo.pRenderPass = m_pBlurRenderPassObject; // Gaussian blur shader works under blur pass

		std::shared_ptr<GraphicsPipelineObject> pBlurPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(blurPipelineCreateInfo, pBlurPipeline);

		GraphicsPipelineCreateInfo lineDrawingBlurPipelineCreateInfo = {};
		lineDrawingBlurPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingBlurPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
		lineDrawingBlurPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingBlurPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		lineDrawingBlurPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingBlurPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingBlurPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingBlurPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingBlurPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Gaussian blur shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pLineDrawingBlurPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingBlurPipelineCreateInfo, pLineDrawingBlurPipeline);

		PassGraphicsPipeline blurPipelines;
		blurPipelines[HPForwardGraphRes::PASSNAME_BLUR] = pBlurPipeline;
		blurPipelines[HPForwardGraphRes::PASSNAME_LINEDRAWING] = pLineDrawingBlurPipeline;

		m_graphicsPipelines[EBuiltInShaderProgramType::GaussianBlur] = blurPipelines;
	}

	// With shadow map shader
	{
		PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.width = 4096; // Correspond to shadow map resolution
		viewportStateCreateInfo.height = 4096;

		std::shared_ptr<PipelineViewportState> pShadowMapViewportState;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pShadowMapViewportState);

		GraphicsPipelineCreateInfo shadowMapPipelineCreateInfo = {};
		shadowMapPipelineCreateInfo.deviceType = EGPUType::Discrete;
		shadowMapPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		shadowMapPipelineCreateInfo.pVertexInputState = pVertexInputState;
		shadowMapPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		shadowMapPipelineCreateInfo.pRasterizationState = pRasterizationState;
		shadowMapPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		shadowMapPipelineCreateInfo.pMultisampleState = pMultisampleState;
		shadowMapPipelineCreateInfo.pViewportState = pShadowMapViewportState;
		shadowMapPipelineCreateInfo.pRenderPass = m_pShadowMapRenderPassObject; // Shadow map shader works under shadow map pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(shadowMapPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline shadowMapPipelines;
		shadowMapPipelines[HPForwardGraphRes::PASSNAME_SHADOWMAP] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::ShadowMap] = shadowMapPipelines;
	}

	// With DOF shader
	{
		GraphicsPipelineCreateInfo dofPipelineCreateInfo = {};
		dofPipelineCreateInfo.deviceType = EGPUType::Discrete;
		dofPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		dofPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		dofPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		// No color blend state
		dofPipelineCreateInfo.pRasterizationState = pRasterizationState;
		dofPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		dofPipelineCreateInfo.pMultisampleState = pMultisampleState;
		dofPipelineCreateInfo.pViewportState = pViewportState;
		dofPipelineCreateInfo.pRenderPass = m_pDOFRenderPassObject; // DOF shader works under DOF pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(dofPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline dofPipelines;
		dofPipelines[HPForwardGraphRes::PASSNAME_DOF] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::DOF] = dofPipelines;
	}
}

void HPForwardRenderer::BuildShadowMapPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_SHADOWMAP, m_pShadowMapPassFrameBuffer);
	passInput.Add(HPForwardGraphRes::RPO_SHADOWMAP, m_pShadowMapRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);

	passOutput.Add(HPForwardGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);

	auto pShadowMapPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Set pipline and render pass
			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::ShadowMap, HPForwardGraphRes::PASSNAME_SHADOWMAP));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_SHADOWMAP)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_SHADOWMAP)));

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);

			UBTransformMatrices ubTransformMatrices;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));
			
			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				// Update uniform blocks

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

				ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
				pLightSpaceTransformMatrixUB->UpdateBufferData(&ubLightSpaceTransformMatrix);

				auto subMeshes = pMesh->GetSubMeshes();
				for (unsigned int i = 0; i < subMeshes->size(); ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, pLightSpaceTransformMatrixUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}
			}

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_SHADOWMAP, pShadowMapPass);
}

void HPForwardRenderer::BuildNormalOnlyPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_NORMALONLY, m_pNormalOnlyPassFrameBuffer);
	passInput.Add(HPForwardGraphRes::RPO_NORMALONLY, m_pNormalOnlyRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);

	passOutput.Add(HPForwardGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passOutput.Add(HPForwardGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);

	auto pNormalOnlyPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::NormalOnly, HPForwardGraphRes::PASSNAME_NORMAL_ONLY));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_NORMALONLY)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_NORMALONLY)));

			// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);

			UBTransformMatrices ubTransformMatrices;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_TRANSFORM_MATRICES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
				Matrix3x3 normalMat = pTransformComp->GetNormalMatrix();

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				auto subMeshes = pMesh->GetSubMeshes();
				for (size_t i = 0; i < subMeshes->size(); ++i)
				{
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}
			}

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pShadowMapPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_SHADOWMAP);
	pShadowMapPass->AddNextNode(pNormalOnlyPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_NORMALONLY, pNormalOnlyPass);
}

void HPForwardRenderer::BuildOpaquePass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_OPAQUE, m_pOpaquePassFrameBuffer);
	passInput.Add(HPForwardGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passInput.Add(HPForwardGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);
	passInput.Add(HPForwardGraphRes::RPO_OPAQUE, m_pOpaqueRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(HPForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);
	passInput.Add(HPForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(HPForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passOutput.Add(HPForwardGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passOutput.Add(HPForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	auto pOpaquePass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			Vector3 lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto pShadowMapTexture = std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_SHADOWMAP_DEPTH));
			
			// TODO: add support for various shaders in this pass
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::AnimeStyle);

			// Alert: it is currently forced to use anime style shader in opaque pass
			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::AnimeStyle, HPForwardGraphRes::PASSNAME_OPAQUE));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_OPAQUE)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_OPAQUE)));

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			UBCameraProperties ubCameraProperties;
			UBMaterialNumericalProperties ubMaterialNumericalProperties;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_SYSTEM_VARIABLES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CAMERA_PROPERTIES));
			auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			ubSystemVariables.timeInSec = Timer::Now();
			pSystemVariablesUB->UpdateBufferData(&ubSystemVariables);

			ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
			pLightSpaceTransformMatrixUB->UpdateBufferData(&ubLightSpaceTransformMatrix);

			ubCameraProperties.cameraPosition = cameraPos;
			pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				unsigned int submeshCount = pMesh->GetSubmeshCount();
				auto subMeshes = pMesh->GetSubMeshes();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				for (unsigned int i = 0; i < submeshCount; ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, pSystemVariablesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EDescriptorType::CombinedImageSampler, pShadowMapTexture);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, pLightSpaceTransformMatrixUB);

					ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
					ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
					ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();
					pMaterialNumericalPropertiesUB->UpdateBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, pMaterialNumericalPropertiesUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
					if (pNoiseTexture)
					{
						pNoiseTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
					}

					auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
					if (pToneTexture)
					{
						pToneTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TONE_TEXTURE), EDescriptorType::CombinedImageSampler, pToneTexture);
					}

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler,
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_NORMALONLY_NORMAL)));

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}

			}

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pNormalOnlyPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_NORMALONLY);
	pNormalOnlyPass->AddNextNode(pOpaquePass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_OPAQUE, pOpaquePass);
}

void HPForwardRenderer::BuildGaussianBlurPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_BLUR_HORI, m_pBlurPassFrameBuffer_Horizontal);
	passInput.Add(HPForwardGraphRes::FB_BLUR_FIN, m_pBlurPassFrameBuffer_FinalColor);
	passInput.Add(HPForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_BLUR_HORIZONTAL_COLOR, m_pBlurPassHorizontalColorOutput);
	passInput.Add(HPForwardGraphRes::RPO_BLUR, m_pBlurRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(HPForwardGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);

	auto pGaussianBlurPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::GaussianBlur, HPForwardGraphRes::PASSNAME_BLUR));

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CONTROL_VARIABLES));

			// Horizontal subpass
			
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_BLUR)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_BLUR_HORI)));

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_OPAQUE_COLOR)));

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Vertical subpass

			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_BLUR)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_BLUR_FIN)));

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_BLUR_HORIZONTAL_COLOR)));

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pOpaquePass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_OPAQUE);
	pOpaquePass->AddNextNode(pGaussianBlurPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_BLUR, pGaussianBlurPass);
}

void HPForwardRenderer::BuildLineDrawingPass()
{
	CreateLineDrawingMatrices(); // Pre-calculate local parameterization matrices

	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_LINEDRAWING_CURV, m_pLineDrawingPassFrameBuffer_Curvature);
	passInput.Add(HPForwardGraphRes::FB_LINEDRAWING_RIDG, m_pLineDrawingPassFrameBuffer_RidgeSearch);
	passInput.Add(HPForwardGraphRes::FB_LINEDRAWING_BLUR_HORI, m_pLineDrawingPassFrameBuffer_Blur_Horizontal);
	passInput.Add(HPForwardGraphRes::FB_LINEDRAWING_BLUR_FIN, m_pLineDrawingPassFrameBuffer_Blur_FinalColor);
	passInput.Add(HPForwardGraphRes::FB_LINEDRAWING_FIN, m_pLineDrawingPassFrameBuffer_FinalBlend);
	passInput.Add(HPForwardGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);
	passInput.Add(HPForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE, m_pLineDrawingMatrixTexture);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_CURVATURE, m_pLineDrawingPassCurvatureOutput);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_BLURRED, m_pLineDrawingPassBlurredOutput);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardGraphRes::RPO_LINEDRAWING, m_pLineDrawingRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(HPForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);

	auto pLineDrawingPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CONTROL_VARIABLES));

			// Curvature computation subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Curvature, HPForwardGraphRes::PASSNAME_LINEDRAWING));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_LINEDRAWING_CURV)));

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_BLUR_FINAL_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SAMPLE_MATRIX_TEXTURE), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<RenderTexture>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Ridge searching subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Color, HPForwardGraphRes::PASSNAME_LINEDRAWING));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_LINEDRAWING_RIDG)));

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_CURVATURE)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Gaussian blur subpasses

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::GaussianBlur, HPForwardGraphRes::PASSNAME_LINEDRAWING));

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);

			// Horizontal subpass

			// Set render target to curvature color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_LINEDRAWING_BLUR_HORI)));

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_COLOR)));

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Vertical subpass

			// Set render target to blurred color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_LINEDRAWING_BLUR_FIN)));

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_CURVATURE)));

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Final blend subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Blend, HPForwardGraphRes::PASSNAME_LINEDRAWING));

			// Set render target to color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_LINEDRAWING_FIN)));

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_BLURRED)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_OPAQUE_COLOR)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

		},
		passInput,
		passOutput);

	auto pBlurPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_BLUR);
	pBlurPass->AddNextNode(pLineDrawingPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_LINEDRAWING, pLineDrawingPass);
}

void HPForwardRenderer::BuildTransparentPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_TRANSP, m_pTranspPassFrameBuffer);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(HPForwardGraphRes::RPO_TRANSP, m_pTranspRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(HPForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(HPForwardGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passOutput.Add(HPForwardGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pTransparentPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBCameraProperties ubCameraProperties;
			UBMaterialNumericalProperties ubMaterialNumericalProperties;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_SYSTEM_VARIABLES));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CAMERA_PROPERTIES));
			auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			ubSystemVariables.timeInSec = Timer::Now();
			pSystemVariablesUB->UpdateBufferData(&ubSystemVariables);

			ubCameraProperties.cameraPosition = cameraPos;
			pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

			// Alert: it is currently forced to use basic transparent shader in transparent pass
			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::Basic_Transparent, HPForwardGraphRes::PASSNAME_TRANSPARENT));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_TRANSP)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_TRANSP)));

			// TODO: add support for various shaders in this pass
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::Basic_Transparent);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				unsigned int submeshCount = pMesh->GetSubmeshCount();
				auto subMeshes = pMesh->GetSubMeshes();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				for (unsigned int i = 0; i < submeshCount; ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (!pMaterial->IsTransparent()) // TODO: use a list to record these parts instead of checking one-by-one
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, pSystemVariablesUB);

					ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
					pMaterialNumericalPropertiesUB->UpdateBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, pMaterialNumericalPropertiesUB);

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_OPAQUE_DEPTH)));

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_COLOR)));

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
					if (pNoiseTexture)
					{
						pNoiseTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
					}

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}
			}

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pLineDrawingPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_LINEDRAWING);
	pLineDrawingPass->AddNextNode(pTransparentPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_TRANSP, pTransparentPass);
}

void HPForwardRenderer::BuildOpaqueTranspBlendPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_BLEND, m_pBlendPassFrameBuffer);
	passInput.Add(HPForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(HPForwardGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);
	passInput.Add(HPForwardGraphRes::TX_BLEND_RPO, m_pBlendRenderPassObject);

	passOutput.Add(HPForwardGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);

	auto pBlendPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::DepthBased_ColorBlend_2, HPForwardGraphRes::PASSNAME_BLEND));
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::TX_BLEND_RPO)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_BLEND)));

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_OPAQUE_DEPTH)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_LINEDRAWING_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_TRANSP_DEPTH)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_TRANSP_COLOR)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pTransparentPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_TRANSP);
	pTransparentPass->AddNextNode(pBlendPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_COLORBLEND_D2, pBlendPass);
}

void HPForwardRenderer::BuildDepthOfFieldPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardGraphRes::FB_DOF_HORI, m_pDOFPassFrameBuffer_Horizontal);
	passInput.Add(HPForwardGraphRes::FB_DOF_FIN, m_pDOFPassPresentFrameBuffers);
	passInput.Add(HPForwardGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);
	passInput.Add(HPForwardGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);
	passInput.Add(HPForwardGraphRes::TX_DOF_HORIZONTAL, m_pDOFPassHorizontalOutput);
	passInput.Add(HPForwardGraphRes::TX_BRUSH_MASK_TEXTURE_1, m_pBrushMaskImageTexture_1);
	passInput.Add(HPForwardGraphRes::TX_BRUSH_MASK_TEXTURE_2, m_pBrushMaskImageTexture_2);
	passInput.Add(HPForwardGraphRes::TX_PENCIL_MASK_TEXTURE_1, m_pPencilMaskImageTexture_1);
	passInput.Add(HPForwardGraphRes::TX_PENCIL_MASK_TEXTURE_2, m_pPencilMaskImageTexture_2);
	passInput.Add(HPForwardGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passInput.Add(HPForwardGraphRes::RPO_DOF, m_pDOFRenderPassObject);
	passInput.Add(HPForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(HPForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	auto pDOFPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			pDevice->BindGraphicsPipeline(((HPForwardRenderer*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::DOF, HPForwardGraphRes::PASSNAME_DOF));

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBCameraProperties ubCameraProperties;
			UBControlVariables ubControlVariables;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_SYSTEM_VARIABLES));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CAMERA_PROPERTIES));
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardGraphRes::UB_CONTROL_VARIABLES));

			ubTransformMatrices.viewMatrix = viewMat;
			pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

			ubSystemVariables.timeInSec = Timer::Now();
			pSystemVariablesUB->UpdateBufferData(&ubSystemVariables);

			ubCameraProperties.aperture = pCameraComp->GetAperture();
			ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
			ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
			pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

			// Horizontal subpass

			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_DOF)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardGraphRes::FB_DOF_HORI)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_BLEND_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_NORMALONLY_POSITION)));

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();

			// Vertical subpass

			// Set to swapchain image output
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardGraphRes::RPO_DOF)),
				std::static_pointer_cast<SwapchainFrameBuffers>(input.Get(HPForwardGraphRes::FB_DOF_FIN))->frameBuffers[pDevice->GetSwapchainPresentImageIndex()]);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_DOF_HORIZONTAL)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_NORMALONLY_POSITION)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, pSystemVariablesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_BRUSH_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_BRUSH_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_PENCIL_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_PENCIL_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardGraphRes::TX_OPAQUE_SHADOW)));

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pDevice->EndRenderPass();
		},
		passInput,
		passOutput);

	auto pBlendPass = m_pRenderGraph->GetNodeByName(HPForwardGraphRes::NODE_COLORBLEND_D2);
	pBlendPass->AddNextNode(pDOFPass);

	m_pRenderGraph->AddRenderNode(HPForwardGraphRes::NODE_DOF, pDOFPass);
}

void HPForwardRenderer::CreateLineDrawingMatrices()
{
	// Alert: current implementation does not support window scaling

	auto width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	auto height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Storing the 6x9 sampling matrix
	// But a5 is not used, so we are only storing 5x9
	std::vector<float> linearResults;
	linearResults.resize(12 * 4, 0);
	m_pLineDrawingMatrixTexture = std::make_shared<RenderTexture>(12, 1); // Store the matrix in a 4-channel linear texture

	float xPixelSize = 1.0f / width;
	float yPixelSize = 1.0f / height;

	Eigen::MatrixXf H(6, 9);
	Eigen::MatrixXf X(9, 6);

	// 3x3 grid sample
	int rowIndex = 0;
	int lineWidth = 1;
	for (int m = -lineWidth; m < lineWidth + 1; m += lineWidth)
	{
		for (int n = -lineWidth; n < lineWidth + 1; n += lineWidth)
		{
			float xSample = m * xPixelSize;
			float ySample = n * yPixelSize;

			X(rowIndex, 0) = xSample * xSample;
			X(rowIndex, 1) = 2 * xSample * ySample;
			X(rowIndex, 2) = ySample * ySample;
			X(rowIndex, 3) = xSample;
			X(rowIndex, 4) = ySample;
			X(rowIndex, 5) = 1;

			rowIndex++;
		}
	}

	H = (X.transpose() * X).inverse() * X.transpose();

	// H is divided as
	// | 1 | 2 |   |
	// | 3 | 4 |   |
	// | 5 | 6 | 11|
	// | 7 | 8 |---|
	// | 9 | 10| 12|

	int linearIndex = 0;
	for (int p = 0; p < 5; ++p) // a5 is not used, so instead of 6 rows, 5 rows are taken
	{
		for (int q = 0; q < 8; ++q)
		{
			linearResults[linearIndex] = H(p, q);
			linearIndex++;
		}
	}

	// Store the 9th column
	for (int p = 0; p < 5; ++p)
	{
		linearResults[linearIndex] = H(p, 8);
		linearIndex++;
	}

	m_pLineDrawingMatrixTexture->FlushData(linearResults.data(), EDataType::Float32, ETextureFormat::RGBA32F);
	m_pLineDrawingMatrixTexture->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
}