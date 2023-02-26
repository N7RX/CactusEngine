#pragma once
#include "DeferredLightingRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* DeferredLightingRenderNode::OUTPUT_COLOR_TEXTURE = "DeferredColorTexture";

	const char* DeferredLightingRenderNode::INPUT_GBUFFER_COLOR = "DeferredInputGColor";
	const char* DeferredLightingRenderNode::INPUT_GBUFFER_NORMAL = "DeferredInputGNormal";
	const char* DeferredLightingRenderNode::INPUT_GBUFFER_POSITION = "DeferredInputGPosition";
	const char* DeferredLightingRenderNode::INPUT_DEPTH_TEXTURE = "DeferredInputDepth";

	DeferredLightingRenderNode::DeferredLightingRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
		: RenderNode(pGraphResources, pRenderer)
	{
		m_inputResourceNames[INPUT_GBUFFER_COLOR] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_NORMAL] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
		m_inputResourceNames[INPUT_DEPTH_TEXTURE] = nullptr;
	}

	void DeferredLightingRenderNode::SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources)
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
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

		// Frame buffer

		FrameBufferCreateInfo fbCreateInfo = {};
		fbCreateInfo.attachments.emplace_back(m_pColorOutput);
		fbCreateInfo.framebufferWidth = screenWidth;
		fbCreateInfo.framebufferHeight = screenHeight;
		fbCreateInfo.pRenderPass = m_pRenderPassObject;

		m_pDevice->CreateFrameBuffer(fbCreateInfo, m_pFrameBuffer);

		// Uniform buffer

		uint32_t perLightSourceAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 1024 : 1;

		UniformBufferCreateInfo ubCreateInfo = {};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * perLightSourceAllocation;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraProperties_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBLightSourceProperties) * perLightSourceAllocation;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pLightSourceProperties_UB);

		// Pipeline object

		// Vertex input states

		VertexInputBindingDescription vertexInputBindingDesc = {};
		vertexInputBindingDesc.binding = 0;
		vertexInputBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
		vertexInputBindingDesc.inputRate = EVertexInputRate::PerVertex;

		VertexInputAttributeDescription positionAttributeDesc = {};
		positionAttributeDesc.binding = vertexInputBindingDesc.binding;
		positionAttributeDesc.location = GraphicsDevice::ATTRIB_POSITION_LOCATION;
		positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
		positionAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription normalAttributeDesc = {};
		normalAttributeDesc.binding = vertexInputBindingDesc.binding;
		normalAttributeDesc.location = GraphicsDevice::ATTRIB_NORMAL_LOCATION;
		normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
		normalAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription texcoordAttributeDesc = {};
		texcoordAttributeDesc.binding = vertexInputBindingDesc.binding;
		texcoordAttributeDesc.location = GraphicsDevice::ATTRIB_TEXCOORD_LOCATION;
		texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
		texcoordAttributeDesc.format = ETextureFormat::RG32F;

		VertexInputAttributeDescription tangentAttributeDesc = {};
		tangentAttributeDesc.binding = vertexInputBindingDesc.binding;
		tangentAttributeDesc.location = GraphicsDevice::ATTRIB_TANGENT_LOCATION;
		tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
		tangentAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription bitangentAttributeDesc = {};
		bitangentAttributeDesc.binding = vertexInputBindingDesc.binding;
		bitangentAttributeDesc.location = GraphicsDevice::ATTRIB_BITANGENT_LOCATION;
		bitangentAttributeDesc.offset = VertexBufferCreateInfo::bitangentOffset;
		bitangentAttributeDesc.format = ETextureFormat::RGB32F;

		PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.bindingDescs = { vertexInputBindingDesc };
		vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc, bitangentAttributeDesc };

		std::shared_ptr<PipelineVertexInputState> pVertexInputState = nullptr;
		m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, pVertexInputState);

		PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
		emptyVertexInputStateCreateInfo.bindingDescs = {};
		emptyVertexInputStateCreateInfo.attributeDescs = {};

		std::shared_ptr<PipelineVertexInputState> pEmptyVertexInputState = nullptr;
		m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

		// Input assembly states

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState_Strip = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_Strip);

		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;

		std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState_List = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_List);

		// Rasterization states

		PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
		rasterizationStateCreateInfo.enableDepthClamp = false;
		rasterizationStateCreateInfo.discardRasterizerResults = false;
		rasterizationStateCreateInfo.cullMode = ECullMode::Front;
		rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

		std::shared_ptr<PipelineRasterizationState> pCullFrontRasterizationState = nullptr;
		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pCullFrontRasterizationState);

		rasterizationStateCreateInfo.cullMode = ECullMode::Back;

		std::shared_ptr<PipelineRasterizationState> pCullBackRasterizationState = nullptr;
		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pCullBackRasterizationState);

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

		AttachmentColorBlendStateDescription attachmentBlendDesc = {};
		attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::One;
		attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::One;
		attachmentBlendDesc.enableBlend = true;

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		AttachmentColorBlendStateDescription attachmentNoBlendDesc = {};
		attachmentNoBlendDesc.enableBlend = false;

		PipelineColorBlendStateCreateInfo colorNoBlendStateCreateInfo = {};
		colorNoBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorNoBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorNoBlendStateCreateInfo, pColorNoBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.width = screenWidth;
		viewportStateCreateInfo.height = screenHeight;

		std::shared_ptr<PipelineViewportState> pViewportState;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

		// Pipeline creation

		GraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);
		pipelineCreateInfo.pVertexInputState = pVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pCullFrontRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DeferredLighting, pPipeline);

		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
		pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		pipelineCreateInfo.pColorBlendState = pColorNoBlendState;
		pipelineCreateInfo.pRasterizationState = pCullBackRasterizationState;

		std::shared_ptr<GraphicsPipelineObject> pDirPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pDirPipeline);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DeferredLighting_Directional, pDirPipeline);
	}

	void DeferredLightingRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
	{
		m_pTransformMatrices_UB->ResetSubBufferAllocation();
		m_pLightSourceProperties_UB->ResetSubBufferAllocation();

		auto pGBufferColorTexture = std::static_pointer_cast<Texture2D>(pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_COLOR)));
		auto pGBufferNormalTexture = std::static_pointer_cast<Texture2D>(pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_NORMAL)));
		auto pGBufferPositionTexture = std::static_pointer_cast<Texture2D>(pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION)));
		auto pSceneDepthTexture = std::static_pointer_cast<Texture2D>(pGraphResources->Get(m_inputResourceNames.at(INPUT_DEPTH_TEXTURE)));

		std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer, pCommandBuffer);

		// Directional light pass (pass through)

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DeferredLighting_Directional), pCommandBuffer);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
		auto pShaderParamTable_ext = std::make_shared<ShaderParameterTable>();

		pShaderParamTable_ext->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GCOLOR_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferColorTexture);

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable_ext, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		if (m_eGraphicsDeviceType != EGraphicsAPIType::Vulkan)
		{
			pShaderProgram->Reset();
		}

		// Other light sources pass

		auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Transform));
		auto pCameraComp = std::static_pointer_cast<CameraComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Camera));
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DeferredLighting), pCommandBuffer);

		UBTransformMatrices ubTransformMatrices = {};
		UBCameraProperties ubCameraProperties = {};
		UBLightSourceProperties ubLightSourceProperties = {};

		pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);

		ubTransformMatrices.projectionMatrix = projectionMat;
		ubTransformMatrices.viewMatrix = viewMat;

		ubCameraProperties.cameraPosition = cameraPos;
		m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pLightComp = std::static_pointer_cast<LightComponent>(entity->GetComponent(EComponentType::Light));
			auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));

			if (!pLightComp || !pTransformComp)
			{
				continue;
			}

			auto lightProfile = pLightComp->GetProfile();

			if (lightProfile.sourceType != LightComponent::SourceType::Directional && !lightProfile.pVolumeMesh)
			{
				continue;
			}

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();

			std::shared_ptr<SubUniformBuffer> pSubTransformMatricesUB = nullptr;
			if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
			{
				pSubTransformMatricesUB = m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
			}
			else
			{
				m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);
			}

			ubLightSourceProperties.source = Vector4(pTransformComp->GetPosition(), (int)lightProfile.sourceType);
			ubLightSourceProperties.color = Color4(lightProfile.lightColor, 1.0f);
			ubLightSourceProperties.intensity = lightProfile.lightIntensity;
			ubLightSourceProperties.radius = lightProfile.radius;

			std::shared_ptr<SubUniformBuffer> pSubLightSourcePropertiesUB = nullptr;
			if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
			{
				pSubLightSourcePropertiesUB = m_pLightSourceProperties_UB->AllocateSubBuffer(sizeof(UBLightSourceProperties));
				pSubLightSourcePropertiesUB->UpdateSubBufferData(&ubLightSourceProperties);
			}
			else
			{
				m_pLightSourceProperties_UB->UpdateBufferData(&ubLightSourceProperties);
			}

			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			unsigned int submeshCount = lightProfile.pVolumeMesh->GetSubmeshCount();
			auto subMeshes = lightProfile.pVolumeMesh->GetSubMeshes();

			m_pDevice->SetVertexBuffer(lightProfile.pVolumeMesh->GetVertexBuffer(), pCommandBuffer);

			for (unsigned int i = 0; i < submeshCount; ++i)
			{
				pShaderParamTable->Clear();

				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, m_pCameraProperties_UB);

				if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSOURCE_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubLightSourcePropertiesUB);
				}
				else
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSOURCE_PROPERTIES), EDescriptorType::UniformBuffer, m_pLightSourceProperties_UB);
				}

				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GCOLOR_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferColorTexture);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferNormalTexture);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferPositionTexture);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, pSceneDepthTexture);

				m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
				m_pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
			}
		}

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