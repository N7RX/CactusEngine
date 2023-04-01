#pragma once
#include "DepthOfFieldRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"
#include "ImageTexture.h"
#include "Timer.h"

namespace Engine
{
	const char* DepthOfFieldRenderNode::INPUT_COLOR_TEXTURE = "DOFInputColorTexture";
	const char* DepthOfFieldRenderNode::INPUT_GBUFFER_POSITION = "DOFInputGBufferPosition";

	DepthOfFieldRenderNode::DepthOfFieldRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject_Horizontal(nullptr),
		m_pRenderPassObject_Vertical(nullptr)
	{
		m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
	}

	void DepthOfFieldRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		// Render pass object

		// Horizontal pass

		RenderPassAttachmentDescription colorDesc{};
		colorDesc.format = ETextureFormat::RGBA8_SRGB;
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

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject_Horizontal);

		// Vertical + present pass

		colorDesc.format = m_outputToSwapchain ? ETextureFormat::BGRA8_UNORM : ETextureFormat::RGBA8_SRGB;
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

		passCreateInfo.attachmentDescriptions.clear();
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject_Vertical);

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
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject_Horizontal;

		GraphicsPipelineObject* pPipeline_0 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_0);

		pipelineCreateInfo.pRenderPass = m_pRenderPassObject_Vertical;

		GraphicsPipelineObject* pPipeline_1 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_1);

		m_graphicsPipelines.emplace((uint32_t)EBuiltInShaderProgramType::DOF, pPipeline_0);
		m_graphicsPipelines.emplace(VERTICAL_PASS_PIPELINE_KEY, pPipeline_1);
	}

	void DepthOfFieldRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
		CreateMutableBuffers(initInfo);
	}

	void DepthOfFieldRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		uint32_t height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		// Horizontal result

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
		texCreateInfo.textureWidth = width;
		texCreateInfo.textureHeight = height;
		texCreateInfo.dataType = EDataType::UByte;
		texCreateInfo.format = ETextureFormat::RGBA8_SRGB;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pHorizontalResult);
		}

		if (!m_outputToSwapchain)
		{
			for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
			{
				m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pVerticalResult);
			}
		}

		// Frame buffers

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pHorizontalResult);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject_Horizontal;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer_Horizontal);
		}

		for (uint32_t i = 0; i < initInfo.framesInFlight; i++)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_outputToSwapchain ? m_pSwapchainImages->at(i) : m_frameResources[i].m_pVerticalResult);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject_Vertical;
			fbCreateInfo.renderToSwapchain = m_outputToSwapchain;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer_Vertical);
		}
	}

	void DepthOfFieldRenderNode::CreateMutableBuffers(const RenderNodeConfiguration& initInfo)
	{
		// Uniform buffers

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBCameraMatrices);
		ubCreateInfo.maxSubAllocationCount = 1;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBControlVariables);
		ubCreateInfo.maxSubAllocationCount = 8;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pControlVariables_UB);
		}
	}

	void DepthOfFieldRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto pCameraTransform = (TransformComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}

		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);
		ShaderParameterTable shaderParamTable{};

		// Prepare shared uniform buffers

		frameResources.m_pControlVariables_UB->ResetSubBufferAllocation();

		UBCameraMatrices ubCameraMatrices{};
		UBCameraProperties ubCameraProperties{};

		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		ubCameraMatrices.viewMatrix = viewMat;
		frameResources.m_pCameraMatrices_UB->UpdateBufferData(&ubCameraMatrices);

		ubCameraProperties.aperture = pCameraComp->GetAperture();
		ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
		ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		// Execute internal passes

		HorizontalPass(pGraphResources, pCommandBuffer, shaderParamTable);
		shaderParamTable.Clear();
		VerticalPass(pGraphResources, pCommandBuffer, shaderParamTable);

		// Submission

		m_pDevice->EndCommandBuffer(pCommandBuffer);

		frameResources.m_pCameraMatrices_UB->FlushToDevice();
		frameResources.m_pCameraProperties_UB->FlushToDevice();
		frameResources.m_pControlVariables_UB->FlushToDevice();

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void DepthOfFieldRenderNode::HorizontalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable)
	{
		auto& frameResources = m_frameResources[m_frameIndex];
		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);

		// Prepare uniform
		UBControlVariables ubControlVariables{};
		ubControlVariables.bool_1 = 1;
		SubUniformBuffer subControlVariablesUB = frameResources.m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
		frameResources.m_pControlVariables_UB->UpdateBufferData(&ubControlVariables, &subControlVariablesUB);

		// Begin pass
		m_pDevice->BeginRenderPass(m_pRenderPassObject_Horizontal, frameResources.m_pFrameBuffer_Horizontal, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at((uint32_t)EBuiltInShaderProgramType::DOF), pCommandBuffer);

		// Update shader resources

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_MATRICES), EDescriptorType::UniformBuffer, frameResources.m_pCameraMatrices_UB);
		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pCameraProperties_UB);

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));
		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, &subControlVariablesUB);

		m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);

		// Draw
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);
		m_pDevice->EndRenderPass(pCommandBuffer);
	}

	void DepthOfFieldRenderNode::VerticalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable)
	{
		auto& frameResources = m_frameResources[m_frameIndex];
		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);

		// Prepare uniform
		UBControlVariables ubControlVariables{};
		ubControlVariables.bool_1 = 0;
		SubUniformBuffer subControlVariablesUB = frameResources.m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
		frameResources.m_pControlVariables_UB->UpdateBufferData(&ubControlVariables, &subControlVariablesUB);

		// Begin pass
		m_pDevice->BeginRenderPass(m_pRenderPassObject_Vertical, frameResources.m_pFrameBuffer_Vertical, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(VERTICAL_PASS_PIPELINE_KEY), pCommandBuffer);

		// Update shader resources

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_MATRICES), EDescriptorType::UniformBuffer, frameResources.m_pCameraMatrices_UB);
		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pCameraProperties_UB);

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, frameResources.m_pHorizontalResult);
		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, &subControlVariablesUB);

		m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);

		// Draw
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);
		m_pDevice->EndRenderPass(pCommandBuffer);
	}

	void DepthOfFieldRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);
	}

	void DepthOfFieldRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;

		if (m_eGraphicsDeviceType != EGraphicsAPIType::OpenGL)
		{
			DestroyMutableBuffers();
			CreateMutableBuffers(m_configuration);
		}
	}

	void DepthOfFieldRenderNode::UpdateFramesInFlight(uint32_t framesInFlight)
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

	void DepthOfFieldRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void DepthOfFieldRenderNode::DestroyConstantResources()
	{
		CE_DELETE(m_pRenderPassObject_Horizontal);
		CE_DELETE(m_pRenderPassObject_Vertical);
		DestroyGraphicsPipelines();
	}

	void DepthOfFieldRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer_Horizontal);
			CE_DELETE(m_frameResources[i].m_pHorizontalResult);

			CE_DELETE(m_frameResources[i].m_pFrameBuffer_Vertical);
			CE_SAFE_DELETE(m_frameResources[i].m_pVerticalResult);
		}
	}

	void DepthOfFieldRenderNode::DestroyMutableBuffers()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pCameraMatrices_UB);
			CE_DELETE(m_frameResources[i].m_pCameraProperties_UB);
			CE_DELETE(m_frameResources[i].m_pControlVariables_UB);
		}
	}
}