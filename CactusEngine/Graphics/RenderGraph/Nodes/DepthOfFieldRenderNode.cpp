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
		m_pRenderPassObject_Present(nullptr),
		m_pFrameBuffers_Present(nullptr)
	{
		m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
	}

	void DepthOfFieldRenderNode::SetupFunction()
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		uint32_t maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();

		m_frameResources.resize(maxFramesInFlight);

		// Horizontal result

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = screenWidth;
		texCreateInfo.textureHeight = screenHeight;
		texCreateInfo.dataType = EDataType::UByte;
		texCreateInfo.format = ETextureFormat::RGBA8_SRGB;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pHorizontalResult);
		}

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

		colorDesc.format = ETextureFormat::BGRA8_UNORM;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::PresentSrc;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::PresentSrc;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		passCreateInfo.attachmentDescriptions.clear();
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject_Present);

		// Frame buffers

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo_Horizontal{};
			fbCreateInfo_Horizontal.attachments.emplace_back(m_frameResources[i].m_pHorizontalResult);
			fbCreateInfo_Horizontal.framebufferWidth = screenWidth;
			fbCreateInfo_Horizontal.framebufferHeight = screenHeight;
			fbCreateInfo_Horizontal.pRenderPass = m_pRenderPassObject_Horizontal;

			m_pDevice->CreateFrameBuffer(fbCreateInfo_Horizontal, m_frameResources[i].m_pFrameBuffer_Horizontal);
		}

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			// Create a framebuffer arround each swapchain image

			CE_NEW(m_pFrameBuffers_Present, SwapchainFrameBuffers);

			std::vector<Texture2D*> swapchainImages;
			m_pDevice->GetSwapchainImages(swapchainImages);

			for (uint32_t i = 0; i < swapchainImages.size(); i++)
			{
				FrameBufferCreateInfo dofFBCreateInfo_Final{};
				dofFBCreateInfo_Final.attachments.emplace_back(swapchainImages[i]);
				dofFBCreateInfo_Final.framebufferWidth = screenWidth;
				dofFBCreateInfo_Final.framebufferHeight = screenHeight;
				dofFBCreateInfo_Final.pRenderPass = m_pRenderPassObject_Present;

				FrameBuffer* pFrameBuffer = nullptr;
				m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Final, pFrameBuffer);
				m_pFrameBuffers_Present->frameBuffers.emplace_back(pFrameBuffer);
			}
		}

		// Uniform buffers

		uint32_t perPassAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 8 : 1;

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pTransformMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBControlVariables) * perPassAllocation;
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pControlVariables_UB);
		}

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
		viewportStateCreateInfo.width = screenWidth;
		viewportStateCreateInfo.height = screenHeight;

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

		pipelineCreateInfo.pRenderPass = m_pRenderPassObject_Present;

		GraphicsPipelineObject* pPipeline_1 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_1);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DOF, pPipeline_0);
		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::NONE, pPipeline_1); // NONE for key only, it will still be using DOF shader
	}

	void DepthOfFieldRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto& frameResource = m_frameResources[m_frameIndex];

		frameResource.m_pControlVariables_UB->ResetSubBufferAllocation();

		auto pCameraTransform = (TransformComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		ShaderParameterTable* pShaderParamTable;
		CE_NEW(pShaderParamTable, ShaderParameterTable);

		UBTransformMatrices ubTransformMatrices{};
		UBSystemVariables ubSystemVariables{};
		UBCameraProperties ubCameraProperties{};
		UBControlVariables ubControlVariables{};

		ubTransformMatrices.viewMatrix = viewMat;
		frameResource.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);

		ubCameraProperties.aperture = pCameraComp->GetAperture();
		ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
		ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
		frameResource.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DOF), pCommandBuffer);

		// Horizontal pass

		m_pDevice->BeginRenderPass(m_pRenderPassObject_Horizontal, frameResource.m_pFrameBuffer_Horizontal, pCommandBuffer);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, frameResource.m_pTransformMatrices_UB);
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResource.m_pCameraProperties_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		ubControlVariables.bool_1 = 1;
		SubUniformBuffer* pSubControlVariablesUB = nullptr;
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			pSubControlVariablesUB = frameResource.m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB);
		}
		else
		{
			frameResource.m_pControlVariables_UB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, frameResource.m_pControlVariables_UB);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			m_pDevice->EndRenderPass(pCommandBuffer);
		}
		else
		{
			pShaderProgram->Reset();
		}

		// Vertical pass

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::NONE), pCommandBuffer);

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			// Set to swapchain image output
			m_pDevice->BeginRenderPass(m_pRenderPassObject_Present, m_pFrameBuffers_Present->frameBuffers[m_pDevice->GetSwapchainPresentImageIndex()], pCommandBuffer);
		}
		else
		{
			// Set to back-buffer output
			m_pDevice->BeginRenderPass(m_pRenderPassObject_Present, nullptr, pCommandBuffer);
		}

		pShaderParamTable->Clear();

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, frameResource.m_pTransformMatrices_UB);
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResource.m_pCameraProperties_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, frameResource.m_pHorizontalResult);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		ubControlVariables.bool_1 = 0;
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			CE_DELETE(pSubControlVariablesUB);
			pSubControlVariablesUB = frameResource.m_pControlVariables_UB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB);
		}
		else
		{
			frameResource.m_pControlVariables_UB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, frameResource.m_pControlVariables_UB);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			m_pDevice->EndRenderPass(pCommandBuffer);
			m_pDevice->EndCommandBuffer(pCommandBuffer);

			m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);

			CE_DELETE(pSubControlVariablesUB);
		}
		else
		{
			pShaderProgram->Reset();
		}

		CE_DELETE(pShaderParamTable);
	}
}