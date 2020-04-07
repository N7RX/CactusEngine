#pragma once
#include "LineDrawingRenderNode.h"
#include "DrawingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"
#include "RenderTexture.h"

#include <Eigen/Dense>

using namespace Engine;

const char* LineDrawingRenderNode::OUTPUT_COLOR_TEXTURE = "LineDrawingOutputColorTexture";

const char* LineDrawingRenderNode::INPUT_COLOR_TEXTURE = "LineDrawingBackgroundColorTexture";
const char* LineDrawingRenderNode::INPUT_LINE_SPACE_TEXTURE = "LineDrawingSpaceColorTexture";

LineDrawingRenderNode::LineDrawingRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
	: RenderNode(pGraphResources, pRenderer), m_enableLineSmooth(false)
{
	m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
	m_inputResourceNames[INPUT_LINE_SPACE_TEXTURE] = nullptr;
}

void LineDrawingRenderNode::SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources)
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Textures

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
	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineResult);

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

	// Frame buffers
	FrameBufferCreateInfo fbCreateInfo_Line = {};
	fbCreateInfo_Line.attachments.emplace_back(m_pLineResult);
	fbCreateInfo_Line.framebufferWidth = screenWidth;
	fbCreateInfo_Line.framebufferHeight = screenHeight;
	fbCreateInfo_Line.pRenderPass = m_pRenderPassObject;

	FrameBufferCreateInfo fbCreateInfo_FinalBlend = {};
	fbCreateInfo_FinalBlend.attachments.emplace_back(m_pColorOutput);
	fbCreateInfo_FinalBlend.framebufferWidth = screenWidth;
	fbCreateInfo_FinalBlend.framebufferHeight = screenHeight;
	fbCreateInfo_FinalBlend.pRenderPass = m_pRenderPassObject;

	m_pDevice->CreateFrameBuffer(fbCreateInfo_Line, m_pFrameBuffer_Line);
	m_pDevice->CreateFrameBuffer(fbCreateInfo_FinalBlend, m_pFrameBuffer_FinalBlend);

	if (m_enableLineSmooth)
	{
		FrameBufferCreateInfo fbCreateInfo_BlurHorizontal = {};
		fbCreateInfo_BlurHorizontal.attachments.emplace_back(m_pColorOutput);
		fbCreateInfo_BlurHorizontal.framebufferWidth = screenWidth;
		fbCreateInfo_BlurHorizontal.framebufferHeight = screenHeight;
		fbCreateInfo_BlurHorizontal.pRenderPass = m_pRenderPassObject;

		FrameBufferCreateInfo fbCreateInfo_BlurFinal = {};
		fbCreateInfo_BlurFinal.attachments.emplace_back(m_pLineResult);
		fbCreateInfo_BlurFinal.framebufferWidth = screenWidth;
		fbCreateInfo_BlurFinal.framebufferHeight = screenHeight;
		fbCreateInfo_BlurFinal.pRenderPass = m_pRenderPassObject;

		m_pDevice->CreateFrameBuffer(fbCreateInfo_BlurHorizontal, m_pFrameBuffer_BlurHorizontal);
		m_pDevice->CreateFrameBuffer(fbCreateInfo_BlurFinal, m_pFrameBuffer_BlurFinal);
	}

	// Uniform buffer

	uint32_t perPassAllocation = m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan ? 8 : 1;

	UniformBufferCreateInfo ubCreateInfo = {};
	ubCreateInfo.sizeInBytes = sizeof(UBControlVariables) * perPassAllocation;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pControlVariables_UB);

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
	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Simplified);
	pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
	pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
	pipelineCreateInfo.pColorBlendState = pColorBlendState;
	pipelineCreateInfo.pRasterizationState = pRasterizationState;
	pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
	pipelineCreateInfo.pMultisampleState = pMultisampleState;
	pipelineCreateInfo.pViewportState = pViewportState;
	pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Simplified = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Simplified);

	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Blend = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Blend);

	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::LineDrawing_Simplified, pPipeline_Simplified);
	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::LineDrawing_Blend, pPipeline_Blend);

	if (m_enableLineSmooth)
	{
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);

		std::shared_ptr<GraphicsPipelineObject> pPipeline_Blur = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Blur);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::GaussianBlur, pPipeline_Blur);
	}
}

void LineDrawingRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
{
	m_pControlVariables_UB->ResetSubBufferAllocation();

	std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr;

	UBControlVariables ubControlVariables = {};

	// Outlining pass

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::LineDrawing_Simplified), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_Line, pCommandBuffer);
	}
	else
	{
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		m_pDevice->SetBlendState(blendInfo);

		m_pDevice->SetRenderTarget(m_pFrameBuffer_Line);
	}

	auto pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Simplified);
	auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_LINE_SPACE_TEXTURE)));

	m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
	m_pDevice->DrawFullScreenQuad(pCommandBuffer);

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->EndRenderPass(pCommandBuffer);
	}
	else
	{
		pShaderProgram->Reset();
	}

	if (m_enableLineSmooth)
	{
		// Gaussian blur passes

		pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);

		// Horizontal pass

		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::GaussianBlur), pCommandBuffer);
			m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_BlurHorizontal, pCommandBuffer);
		}
		else
		{
			m_pDevice->SetRenderTarget(m_pFrameBuffer_BlurHorizontal);
		}

		pShaderParamTable->Clear();

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pLineResult);

		ubControlVariables.bool_1 = 1;
		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			auto pSubControlVariablesUB = m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB);
		}
		else
		{
			m_pControlVariables_UB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, m_pControlVariables_UB);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			m_pDevice->EndRenderPass(pCommandBuffer);
		}
		else
		{
			pShaderProgram->Reset();
		}

		// Vertical pass

		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_BlurFinal, pCommandBuffer);
		}
		else
		{
			m_pDevice->SetRenderTarget(m_pFrameBuffer_BlurFinal);
		}

		pShaderParamTable->Clear();

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pColorOutput);

		ubControlVariables.bool_1 = 0;
		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			auto pSubControlVariablesUB = m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB);
		}
		else
		{
			m_pControlVariables_UB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, m_pControlVariables_UB);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			m_pDevice->EndRenderPass(pCommandBuffer);
		}
		else
		{
			pShaderProgram->Reset();
		}
	}

	// Final blend pass

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::LineDrawing_Blend), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_FinalBlend, pCommandBuffer);
	}
	else
	{
		m_pDevice->SetRenderTarget(m_pFrameBuffer_FinalBlend);
	}

	pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
	pShaderParamTable->Clear();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		m_pLineResult);

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));

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