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
	const char* DepthOfFieldRenderNode::INPUT_SHADOW_MARK_TEXTURE = "DOFInputShadowMarkTexture";

	DepthOfFieldRenderNode::DepthOfFieldRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
		: RenderNode(pGraphResources, pRenderer)
	{
		m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
		m_inputResourceNames[INPUT_SHADOW_MARK_TEXTURE] = nullptr;
	}

	void DepthOfFieldRenderNode::SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources)
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		// Horizontal result

		Texture2DCreateInfo texCreateInfo = {};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = screenWidth;
		texCreateInfo.textureHeight = screenHeight;
		texCreateInfo.dataType = EDataType::Float32;
		texCreateInfo.format = ETextureFormat::RGBA32F;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		m_pDevice->CreateTexture2D(texCreateInfo, m_pHorizontalResult);

		// Post effects images

		m_pBrushMaskTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_1.png");
		m_pBrushMaskTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_2.png");
		m_pPencilMaskTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_1.jpg");
		m_pPencilMaskTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_2.jpg");

		m_pBrushMaskTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler());
		m_pBrushMaskTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler());
		m_pPencilMaskTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler());
		m_pPencilMaskTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler());

		// Render pass object

		// Horizontal pass

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

		FrameBufferCreateInfo fbCreateInfo_Horizontal = {};
		fbCreateInfo_Horizontal.attachments.emplace_back(m_pHorizontalResult);
		fbCreateInfo_Horizontal.framebufferWidth = screenWidth;
		fbCreateInfo_Horizontal.framebufferHeight = screenHeight;
		fbCreateInfo_Horizontal.pRenderPass = m_pRenderPassObject_Horizontal;

		m_pDevice->CreateFrameBuffer(fbCreateInfo_Horizontal, m_pFrameBuffer_Horizontal);

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			// Create a framebuffer arround each swapchain image

			m_pFrameBuffers_Present = std::make_shared<SwapchainFrameBuffers>();

			std::vector<std::shared_ptr<Texture2D>> swapchainImages;
			m_pDevice->GetSwapchainImages(swapchainImages);

			for (unsigned int i = 0; i < swapchainImages.size(); i++)
			{
				FrameBufferCreateInfo dofFBCreateInfo_Final = {};
				dofFBCreateInfo_Final.attachments.emplace_back(swapchainImages[i]);
				dofFBCreateInfo_Final.framebufferWidth = screenWidth;
				dofFBCreateInfo_Final.framebufferHeight = screenHeight;
				dofFBCreateInfo_Final.pRenderPass = m_pRenderPassObject_Present;

				std::shared_ptr<FrameBuffer> pFrameBuffer;
				m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Final, pFrameBuffer);
				m_pFrameBuffers_Present->frameBuffers.emplace_back(pFrameBuffer);
			}
		}

		// Uniform buffers

		uint32_t perPassAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 8 : 1;

		UniformBufferCreateInfo ubCreateInfo = {};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraProperties_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables);
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBControlVariables) * perPassAllocation;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pControlVariables_UB);

		// Pipeline objects

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
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject_Horizontal;

		std::shared_ptr<GraphicsPipelineObject> pPipeline_0 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_0);

		pipelineCreateInfo.pRenderPass = m_pRenderPassObject_Present;

		std::shared_ptr<GraphicsPipelineObject> pPipeline_1 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_1);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DOF, pPipeline_0);
		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::NONE, pPipeline_1); // NONE for key only, it will still be using DOF shader
	}

	void DepthOfFieldRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
	{
		m_pControlVariables_UB->ResetSubBufferAllocation();

		auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Transform));
		auto pCameraComp = std::static_pointer_cast<CameraComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Camera));
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);

		std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		UBTransformMatrices ubTransformMatrices = {};
		UBSystemVariables ubSystemVariables = {};
		UBCameraProperties ubCameraProperties = {};
		UBControlVariables ubControlVariables = {};

		ubTransformMatrices.viewMatrix = viewMat;
		m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);

		ubSystemVariables.timeInSec = Timer::Now();
		m_pSystemVariables_UB->UpdateBufferData(&ubSystemVariables);

		ubCameraProperties.aperture = pCameraComp->GetAperture();
		ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
		ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
		m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DOF), pCommandBuffer);

		// Horizontal pass

		m_pDevice->BeginRenderPass(m_pRenderPassObject_Horizontal, m_pFrameBuffer_Horizontal, pCommandBuffer);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, m_pCameraProperties_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, m_pSystemVariables_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pBrushMaskTexture_1);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pBrushMaskTexture_2);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			m_pPencilMaskTexture_1);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			m_pPencilMaskTexture_2);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_SHADOW_MARK_TEXTURE)));

		ubControlVariables.bool_1 = 1;
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
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

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, m_pCameraProperties_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pHorizontalResult);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, m_pSystemVariables_UB);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pBrushMaskTexture_1);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler,
			m_pBrushMaskTexture_2);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			m_pPencilMaskTexture_1);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			m_pPencilMaskTexture_2);

		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
			pGraphResources->Get(m_inputResourceNames.at(INPUT_SHADOW_MARK_TEXTURE)));

		ubControlVariables.bool_1 = 0;
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
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

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
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
}