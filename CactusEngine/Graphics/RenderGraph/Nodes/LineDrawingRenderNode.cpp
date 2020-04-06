#pragma once
#include "LineDrawingRenderNode.h"
#include "DrawingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"
#include "RenderTexture.h"

#include <Eigen/Dense>

using namespace Engine;

const char* LineDrawingRenderNode::OUTPUT_COLOR_TEXTURE = "LineDrawingOutputColorTexture";

const char* LineDrawingRenderNode::INPUT_COLOR_TEXTURE = "LineDrawingInputColorTexture";
const char* LineDrawingRenderNode::INPUT_BLURRED_COLOR_TEXTURE = "LineDrawingInputBlurredColorTexture";

LineDrawingRenderNode::LineDrawingRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
	: RenderNode(pGraphResources, pRenderer)
{
	m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
	m_inputResourceNames[INPUT_BLURRED_COLOR_TEXTURE] = nullptr;
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
	m_pDevice->CreateTexture2D(texCreateInfo, m_pCurvatureTexture);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurLineResult);

	// Pre-calculate local parameterization matrices
	CreateMatrixTexture();

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
	FrameBufferCreateInfo fbCreateInfo_Curvature = {};
	fbCreateInfo_Curvature.attachments.emplace_back(m_pCurvatureTexture);
	fbCreateInfo_Curvature.framebufferWidth = screenWidth;
	fbCreateInfo_Curvature.framebufferHeight = screenHeight;
	fbCreateInfo_Curvature.pRenderPass = m_pRenderPassObject;

	FrameBufferCreateInfo fbCreateInfo_RidgeSearch = {};
	fbCreateInfo_RidgeSearch.attachments.emplace_back(m_pColorOutput);
	fbCreateInfo_RidgeSearch.framebufferWidth = screenWidth;
	fbCreateInfo_RidgeSearch.framebufferHeight = screenHeight;
	fbCreateInfo_RidgeSearch.pRenderPass = m_pRenderPassObject;

	FrameBufferCreateInfo fbCreateInfo_BlurHorizontal = {};
	fbCreateInfo_BlurHorizontal.attachments.emplace_back(m_pCurvatureTexture);
	fbCreateInfo_BlurHorizontal.framebufferWidth = screenWidth;
	fbCreateInfo_BlurHorizontal.framebufferHeight = screenHeight;
	fbCreateInfo_BlurHorizontal.pRenderPass = m_pRenderPassObject;

	FrameBufferCreateInfo fbCreateInfo_BlurFinal = {};
	fbCreateInfo_BlurFinal.attachments.emplace_back(m_pBlurLineResult);
	fbCreateInfo_BlurFinal.framebufferWidth = screenWidth;
	fbCreateInfo_BlurFinal.framebufferHeight = screenHeight;
	fbCreateInfo_BlurFinal.pRenderPass = m_pRenderPassObject;

	FrameBufferCreateInfo fbCreateInfo_FinalBlend = {};
	fbCreateInfo_FinalBlend.attachments.emplace_back(m_pColorOutput);
	fbCreateInfo_FinalBlend.framebufferWidth = screenWidth;
	fbCreateInfo_FinalBlend.framebufferHeight = screenHeight;
	fbCreateInfo_FinalBlend.pRenderPass = m_pRenderPassObject;

	m_pDevice->CreateFrameBuffer(fbCreateInfo_Curvature, m_pFrameBuffer_Curvature);
	m_pDevice->CreateFrameBuffer(fbCreateInfo_RidgeSearch, m_pFrameBuffer_RidgeSearch);
	m_pDevice->CreateFrameBuffer(fbCreateInfo_BlurHorizontal, m_pFrameBuffer_BlurHorizontal);
	m_pDevice->CreateFrameBuffer(fbCreateInfo_BlurFinal, m_pFrameBuffer_BlurFinal);
	m_pDevice->CreateFrameBuffer(fbCreateInfo_FinalBlend, m_pFrameBuffer_FinalBlend);

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
	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
	pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
	pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
	pipelineCreateInfo.pColorBlendState = pColorBlendState;
	pipelineCreateInfo.pRasterizationState = pRasterizationState;
	pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
	pipelineCreateInfo.pMultisampleState = pMultisampleState;
	pipelineCreateInfo.pViewportState = pViewportState;
	pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Color = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Color);

	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Curvature = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Curvature);

	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Blend = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Blend);

	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);

	std::shared_ptr<GraphicsPipelineObject> pPipeline_Blur = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_Blur);

	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::LineDrawing_Color, pPipeline_Color);
	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::LineDrawing_Curvature, pPipeline_Curvature);
	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::LineDrawing_Blend, pPipeline_Blend);
	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::GaussianBlur, pPipeline_Blur);
}

void LineDrawingRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
{
	m_pControlVariables_UB->ResetSubBufferAllocation();

	std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr;

	UBControlVariables ubControlVariables = {};

	// Curvature computation pass

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::LineDrawing_Curvature), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_Curvature, pCommandBuffer);
	}
	else
	{
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		m_pDevice->SetBlendState(blendInfo);

		m_pDevice->SetRenderTarget(m_pFrameBuffer_Curvature);
		m_pDevice->ClearRenderTarget();
	}

	auto pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
	auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		pGraphResources->Get(m_inputResourceNames.at(INPUT_BLURRED_COLOR_TEXTURE)));

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SAMPLE_MATRIX_TEXTURE), EDescriptorType::CombinedImageSampler,
		m_pMatrixTexture);

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

	// Ridge searching pass

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::LineDrawing_Color), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_RidgeSearch, pCommandBuffer);
	}
	else
	{
		m_pDevice->SetRenderTarget(m_pFrameBuffer_RidgeSearch);
		m_pDevice->ClearRenderTarget();
	}

	pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
	pShaderParamTable->Clear();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		m_pCurvatureTexture);

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
		m_pDevice->ClearRenderTarget();
	}

	pShaderParamTable->Clear();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		m_pColorOutput);

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
		m_pDevice->ClearRenderTarget();
	}

	pShaderParamTable->Clear();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		m_pCurvatureTexture);

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

	// Final blend pass

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::LineDrawing_Blend), pCommandBuffer);
		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer_FinalBlend, pCommandBuffer);
	}
	else
	{
		m_pDevice->SetRenderTarget(m_pFrameBuffer_FinalBlend);
		m_pDevice->ClearRenderTarget();
	}

	pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
	pShaderParamTable->Clear();

	pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
		m_pBlurLineResult);

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

void LineDrawingRenderNode::CreateMatrixTexture()
{
	// Alert: current implementation does not support window scaling

	auto width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	auto height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Storing the 6x9 sampling matrix
	// But a5 is not used, so we are only storing 5x9
	std::vector<float> linearResults;
	linearResults.resize(12 * 4, 0);
	m_pMatrixTexture = std::make_shared<RenderTexture>(12, 1); // Store the matrix in a 4-channel linear texture

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

	std::static_pointer_cast<RenderTexture>(m_pMatrixTexture)->FlushData(linearResults.data(), EDataType::Float32, ETextureFormat::RGBA32F);
	m_pMatrixTexture->SetSampler(m_pDevice->GetDefaultTextureSampler());
}