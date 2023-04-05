#pragma once
#include "TransparencyBlendRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* TransparencyBlendRenderNode::OUTPUT_COLOR_TEXTURE = "TransparencyBlendColorTexture";

	const char* TransparencyBlendRenderNode::INPUT_OPQAUE_COLOR_TEXTURE = "TransparencyQpaqueColorTexture";
	const char* TransparencyBlendRenderNode::INPUT_OPQAUE_DEPTH_TEXTURE = "TransparencyQpaqueDepthTexture";
	const char* TransparencyBlendRenderNode::INPUT_TRANSPARENCY_COLOR_TEXTURE = "TransparencyTransparentColorTexture";
	const char* TransparencyBlendRenderNode::INPUT_TRANSPARENCY_DEPTH_TEXTURE = "TransparencyTransparentDepthTexture";

	TransparencyBlendRenderNode::TransparencyBlendRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject(nullptr)
	{
		m_inputResourceNames[INPUT_OPQAUE_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_OPQAUE_DEPTH_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_TRANSPARENCY_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_TRANSPARENCY_DEPTH_TEXTURE] = nullptr;
	}

	void TransparencyBlendRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		// Render pass object

		RenderPassAttachmentDescription colorDesc{};
		colorDesc.format = m_outputToSwapchain ? initInfo.swapSurfaceFormat : initInfo.colorFormat;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = m_outputToSwapchain ? EImageLayout::PresentSrc : EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = colorDesc.initialLayout;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Pipeline objects

		// Vertex input state

		PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo{};
		emptyVertexInputStateCreateInfo.bindingDescs = {};
		emptyVertexInputStateCreateInfo.attributeDescs = {};

		PipelineVertexInputState* pEmptyVertexInputState = nullptr;
		m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

		// Input assembly state

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		PipelineInputAssemblyState* pInputAssemblyState_Strip = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_Strip);

		// Rasterization state

		PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
		rasterizationStateCreateInfo.enableDepthClamp = false;
		rasterizationStateCreateInfo.discardRasterizerResults = false;
		rasterizationStateCreateInfo.cullMode = ECullMode::Back;
		rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

		PipelineRasterizationState* pRasterizationState = nullptr;
		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pRasterizationState);

		// Depth stencil state

		PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
		depthStencilStateCreateInfo.enableDepthTest = false;
		depthStencilStateCreateInfo.enableDepthWrite = false;
		depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
		depthStencilStateCreateInfo.enableStencilTest = false;

		PipelineDepthStencilState* pDepthStencilState = nullptr;
		m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState);

		// Multisample state

		PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
		multisampleStateCreateInfo.enableSampleShading = false;
		multisampleStateCreateInfo.sampleCount = 1;

		PipelineMultisampleState* pMultisampleState = nullptr;
		m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, pMultisampleState);

		// Color blend state

		AttachmentColorBlendStateDescription attachmentNoBlendDesc{};
		attachmentNoBlendDesc.enableBlend = false;

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		PipelineColorBlendState* pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		viewportStateCreateInfo.height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		PipelineViewportState* pViewportState = nullptr;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

		// Pipeline creation

		GraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

		GraphicsPipelineObject* pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

		m_graphicsPipelines.emplace((uint32_t)EBuiltInShaderProgramType::DepthBased_ColorBlend_2, pPipeline);
	}

	void TransparencyBlendRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
	}

	void TransparencyBlendRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		uint32_t height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		// Color output

		if (!m_outputToSwapchain)
		{
			Texture2DCreateInfo texCreateInfo{};
			texCreateInfo.generateMipmap = false;
			texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
			texCreateInfo.textureWidth = width;
			texCreateInfo.textureHeight = height;
			texCreateInfo.format = initInfo.colorFormat;
			texCreateInfo.textureType = ETextureType::ColorAttachment;
			texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

			for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
			{
				m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
				m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
			}
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_outputToSwapchain ? m_pSwapchainImages->at(i) : m_frameResources[i].m_pColorOutput);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;
			fbCreateInfo.renderToSwapchain = m_outputToSwapchain;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void TransparencyBlendRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext)
	{
		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(cmdContext.pCommandPool);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		ShaderParameterTable shaderParamTable{};

		// Bind pipeline
		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at((uint32_t)EBuiltInShaderProgramType::DepthBased_ColorBlend_2), pCommandBuffer);

		// Update shader resources

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_OPQAUE_DEPTH_TEXTURE)));

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_OPQAUE_COLOR_TEXTURE)));

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_TRANSPARENCY_DEPTH_TEXTURE)));

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_TRANSPARENCY_COLOR_TEXTURE)));

		m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);

		// Draw
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		// End pass

		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void TransparencyBlendRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);
	}

	void TransparencyBlendRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;
	}

	void TransparencyBlendRenderNode::UpdateFramesInFlight(uint32_t framesInFlight)
	{
		if (m_configuration.framesInFlight != framesInFlight)
		{
			m_configuration.framesInFlight = framesInFlight;

			if (framesInFlight < m_frameResources.size())
			{
				m_frameResources.resize(framesInFlight);
			}
			else
			{
				DestroyMutableResources();
				CreateMutableResources(m_configuration);
			}
		}
	}

	void TransparencyBlendRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void TransparencyBlendRenderNode::DestroyConstantResources()
	{
		CE_DELETE(m_pRenderPassObject);
		DestroyGraphicsPipelines();
	}

	void TransparencyBlendRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_SAFE_DELETE(m_frameResources[i].m_pColorOutput);
		}
	}
}