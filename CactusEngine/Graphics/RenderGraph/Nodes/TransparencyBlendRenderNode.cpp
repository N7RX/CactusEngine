#pragma once
#include "TransparencyBlendRenderNode.h"
#include "DrawingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

using namespace Engine;

const char* TransparencyBlendRenderNode::OUTPUT_COLOR_TEXTURE = "TransparencyBlendColorTexture";

const char* TransparencyBlendRenderNode::INPUT_OPQAUE_COLOR_TEXTURE = "TransparencyQpaqueColorTexture";
const char* TransparencyBlendRenderNode::INPUT_OPQAUE_DEPTH_TEXTURE = "TransparencyQpaqueDepthTexture";
const char* TransparencyBlendRenderNode::INPUT_TRANSPARENCY_COLOR_TEXTURE = "TransparencyTransparentColorTexture";
const char* TransparencyBlendRenderNode::INPUT_TRANSPARENCY_DEPTH_TEXTURE = "TransparencyTransparentDepthTexture";


TransparencyBlendRenderNode::TransparencyBlendRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
	: RenderNode(pGraphResources, pRenderer)
{
	m_inputResourceNames[INPUT_OPQAUE_COLOR_TEXTURE] = nullptr;
	m_inputResourceNames[INPUT_OPQAUE_DEPTH_TEXTURE] = nullptr;
	m_inputResourceNames[INPUT_TRANSPARENCY_COLOR_TEXTURE] = nullptr;
	m_inputResourceNames[INPUT_TRANSPARENCY_DEPTH_TEXTURE] = nullptr;
}

void TransparencyBlendRenderNode::SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources)
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Color output

	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.generateMipmap = false;
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
	texCreateInfo.textureWidth = screenWidth;
	texCreateInfo.textureHeight = screenHeight;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pColorOutput);

	pGraphResources->Add(OUTPUT_COLOR_TEXTURE, m_pColorOutput);

	// Render pass object

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		RenderPassCreateInfo passCreateInfo = {};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);
	}

	// Frame buffer

	FrameBufferCreateInfo fbCreateInfo = {};
	fbCreateInfo.attachments.emplace_back(m_pColorOutput);
	fbCreateInfo.framebufferWidth = screenWidth;
	fbCreateInfo.framebufferHeight = screenHeight;
	fbCreateInfo.pRenderPass = m_pRenderPassObject;

	m_pDevice->CreateFrameBuffer(fbCreateInfo, m_pFrameBuffer);

	// Pipeline objects

	if (m_eGraphicsDeviceType != EGraphicsDeviceType::Vulkan)
	{
		return;
	}

	// Vertex input state

	PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
	emptyVertexInputStateCreateInfo.bindingDescs = {};
	emptyVertexInputStateCreateInfo.attributeDescs = {};

	std::shared_ptr<PipelineVertexInputState> pEmptyVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

	// Input assembly state

	PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
	inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

	std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState_Strip = nullptr;
	m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_Strip);

	// Rasterization state

	PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
	rasterizationStateCreateInfo.enableDepthClamp = false;
	rasterizationStateCreateInfo.discardRasterizerResults = false;
	rasterizationStateCreateInfo.cullMode = ECullMode::Back;
	rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

	std::shared_ptr<PipelineRasterizationState> pRasterizationState = nullptr;
	m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pRasterizationState);

	// Depth stencil state

	PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.enableDepthTest = false;
	depthStencilStateCreateInfo.enableDepthWrite = false;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
	depthStencilStateCreateInfo.enableStencilTest = false;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState);

	// Multisample state

	PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.enableSampleShading = false;
	multisampleStateCreateInfo.sampleCount = 1;

	std::shared_ptr<PipelineMultisampleState> pMultisampleState = nullptr;
	m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, pMultisampleState);

	// Color blend state

	AttachmentColorBlendStateDescription attachmentNoBlendDesc = {};
	attachmentNoBlendDesc.enableBlend = false;

	PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

	std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
	m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

	// Viewport state

	PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.width = screenWidth;
	viewportStateCreateInfo.height = screenHeight;

	std::shared_ptr<PipelineViewportState> pViewportState;
	m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

	// Pipeline creation

	GraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
	pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
	pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
	pipelineCreateInfo.pColorBlendState = pColorBlendState;
	pipelineCreateInfo.pRasterizationState = pRasterizationState;
	pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
	pipelineCreateInfo.pMultisampleState = pMultisampleState;
	pipelineCreateInfo.pViewportState = pViewportState;
	pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

	std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DepthBased_ColorBlend_2, pPipeline);
}

void TransparencyBlendRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
{
	std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr;

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DepthBased_ColorBlend_2), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer, pCommandBuffer);
	}
	else
	{
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		m_pDevice->SetBlendState(blendInfo);

		m_pDevice->SetRenderTarget(m_pFrameBuffer);
		m_pDevice->ClearRenderTarget();
	}

	auto pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
	auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_OPQAUE_DEPTH_TEXTURE)));

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_OPQAUE_COLOR_TEXTURE)));

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_2), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_TRANSPARENCY_DEPTH_TEXTURE)));

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_TRANSPARENCY_COLOR_TEXTURE)));

	m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
	m_pDevice->DrawFullScreenQuad(pCommandBuffer);

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}
	else
	{
		pShaderProgram->Reset();
	}
}