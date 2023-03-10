#pragma once
#include "OpaqueContentRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE = "OpaqueColorTexture";
	const char* OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE = "OpaqueDepthTexture";
	const char* OpaqueContentRenderNode::OUTPUT_LINE_SPACE_TEXTURE = "OpaqueLineSpaceTexture";

	const char* OpaqueContentRenderNode::INPUT_GBUFFER_NORMAL = "OpaqueInputGBufferNormal";
	const char* OpaqueContentRenderNode::INPUT_SHADOW_MAP = "OpaqueInputShadowMap";

	OpaqueContentRenderNode::OpaqueContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject(nullptr)
	{
		m_inputResourceNames[INPUT_GBUFFER_NORMAL] = nullptr;
		m_inputResourceNames[INPUT_SHADOW_MAP] = nullptr;
	}

	void OpaqueContentRenderNode::SetupFunction()
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		uint32_t maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();

		m_frameResources.resize(maxFramesInFlight);

		// Color output and shadow mark output

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = screenWidth;
		texCreateInfo.textureHeight = screenHeight;
		texCreateInfo.dataType = EDataType::Float32;
		texCreateInfo.format = ETextureFormat::RGBA32F;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pLineSpaceOutput);
		}

		// Depth output

		texCreateInfo.format = ETextureFormat::Depth;
		texCreateInfo.textureType = ETextureType::DepthAttachment;

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pDepthOutput);

			m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
			m_graphResources[i]->Add(OUTPUT_DEPTH_TEXTURE, m_frameResources[i].m_pDepthOutput);
			m_graphResources[i]->Add(OUTPUT_LINE_SPACE_TEXTURE, m_frameResources[i].m_pLineSpaceOutput);
		}

		// Render pass object

		RenderPassAttachmentDescription colorDesc{};
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

		RenderPassAttachmentDescription shadowDesc{};
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

		RenderPassAttachmentDescription depthDesc{};
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
		depthDesc.index = 2;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);
		passCreateInfo.attachmentDescriptions.emplace_back(shadowDesc);
		passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Frame buffer

		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pColorOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pLineSpaceOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pDepthOutput);
			fbCreateInfo.framebufferWidth = screenWidth;
			fbCreateInfo.framebufferHeight = screenHeight;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}

		// Uniform buffers

		uint32_t perSubmeshAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 4096 : 1;

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * perSubmeshAllocation;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pTransformMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pLightSpaceTransformMatrix_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties) * perSubmeshAllocation;
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pMaterialNumericalProperties_UB);
		}

		// Pipeline objects

		// Vertex input state

		VertexInputBindingDescription vertexInputBindingDesc{};
		vertexInputBindingDesc.binding = 0;
		vertexInputBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
		vertexInputBindingDesc.inputRate = EVertexInputRate::PerVertex;

		VertexInputAttributeDescription positionAttributeDesc{};
		positionAttributeDesc.binding = vertexInputBindingDesc.binding;
		positionAttributeDesc.location = GraphicsDevice::ATTRIB_POSITION_LOCATION;
		positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
		positionAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription normalAttributeDesc{};
		normalAttributeDesc.binding = vertexInputBindingDesc.binding;
		normalAttributeDesc.location = GraphicsDevice::ATTRIB_NORMAL_LOCATION;
		normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
		normalAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription texcoordAttributeDesc{};
		texcoordAttributeDesc.binding = vertexInputBindingDesc.binding;
		texcoordAttributeDesc.location = GraphicsDevice::ATTRIB_TEXCOORD_LOCATION;
		texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
		texcoordAttributeDesc.format = ETextureFormat::RG32F;

		VertexInputAttributeDescription tangentAttributeDesc{};
		tangentAttributeDesc.binding = vertexInputBindingDesc.binding;
		tangentAttributeDesc.location = GraphicsDevice::ATTRIB_TANGENT_LOCATION;
		tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
		tangentAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription bitangentAttributeDesc{};
		bitangentAttributeDesc.binding = vertexInputBindingDesc.binding;
		bitangentAttributeDesc.location = GraphicsDevice::ATTRIB_BITANGENT_LOCATION;
		bitangentAttributeDesc.offset = VertexBufferCreateInfo::bitangentOffset;
		bitangentAttributeDesc.format = ETextureFormat::RGB32F;

		PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.bindingDescs = { vertexInputBindingDesc };
		vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc, bitangentAttributeDesc };

		PipelineVertexInputState* pVertexInputState = nullptr;
		m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, pVertexInputState);

		// Input assembly state

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		PipelineInputAssemblyState* pInputAssemblyState = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState);

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
		depthStencilStateCreateInfo.enableDepthTest = true;
		depthStencilStateCreateInfo.enableDepthWrite = true;
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
		colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);
		colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);

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
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::AnimeStyle);
		pipelineCreateInfo.pVertexInputState = pVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

		GraphicsPipelineObject* pPipeline_0 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_0);

		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic);

		GraphicsPipelineObject* pPipeline_1 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_1);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::AnimeStyle, pPipeline_0);
		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::Basic, pPipeline_1);
	}

	void OpaqueContentRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto& frameResources = m_frameResources[m_frameIndex];

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();
		frameResources.m_pMaterialNumericalProperties_UB->ResetSubBufferAllocation();

		auto pCameraTransform = (TransformComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);

		Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
		Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
		Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
		Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

		auto pGBufferNormalTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_NORMAL));
		auto pShadowMapTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_SHADOW_MAP));

		ShaderProgram* pShaderProgram = nullptr;
		EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;
		ShaderParameterTable* pShaderParamTable;
		CE_NEW(pShaderParamTable, ShaderParameterTable);

		UBTransformMatrices ubTransformMatrices{};
		UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix{};
		UBCameraProperties ubCameraProperties{};
		UBMaterialNumericalProperties ubMaterialNumericalProperties{};

		ubTransformMatrices.projectionMatrix = projectionMat;
		ubTransformMatrices.viewMatrix = viewMat;

		ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
		frameResources.m_pLightSpaceTransformMatrix_UB->UpdateBufferData(&ubLightSpaceTransformMatrix);

		ubCameraProperties.cameraPosition = cameraPos;
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);

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

			SubUniformBuffer* pSubTransformMatricesUB = nullptr;
			if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
			{
				pSubTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
			}
			else
			{
				frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);
			}

			pShaderParamTable->Clear();

			uint32_t submeshCount = pMesh->GetSubmeshCount();
			auto subMeshes = pMesh->GetSubMeshes();

			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

			for (uint32_t i = 0; i < submeshCount; ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

				if (pMaterial->IsTransparent())
				{
					continue;
				}

				if (lastUsedShaderProgramType != pMaterial->GetShaderProgramType())
				{
					m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(pMaterial->GetShaderProgramType()), pCommandBuffer);
					pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
					lastUsedShaderProgramType = pMaterial->GetShaderProgramType();
				}
				pShaderParamTable->Clear();

				if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
				}
				else
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, frameResources.m_pTransformMatrices_UB);
				}
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pCameraProperties_UB);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferNormalTexture);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EDescriptorType::CombinedImageSampler, pShadowMapTexture);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, frameResources.m_pLightSpaceTransformMatrix_UB);

				ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
				ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
				ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();

				SubUniformBuffer* pSubMaterialNumericalPropertiesUB = nullptr;
				if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
				{
					pSubMaterialNumericalPropertiesUB = frameResources.m_pMaterialNumericalProperties_UB->AllocateSubBuffer(sizeof(UBMaterialNumericalProperties));
					pSubMaterialNumericalPropertiesUB->UpdateSubBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubMaterialNumericalPropertiesUB);
				}
				else
				{
					frameResources.m_pMaterialNumericalProperties_UB->UpdateBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pMaterialNumericalProperties_UB);
				}

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetDefaultTextureSampler(true));
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
				if (pToneTexture)
				{
					pToneTexture->SetSampler(m_pDevice->GetDefaultTextureSampler());
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TONE_TEXTURE), EDescriptorType::CombinedImageSampler, pToneTexture);
				}

				m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
				m_pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);

				if (m_eGraphicsDeviceType != EGraphicsAPIType::Vulkan)
				{
					pShaderProgram->Reset();
				}
				else
				{
					CE_DELETE(pSubMaterialNumericalPropertiesUB);
				}
			}

			if (pSubTransformMatricesUB)
			{
				CE_DELETE(pSubTransformMatricesUB);
			}
		}

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			m_pDevice->EndRenderPass(pCommandBuffer);
			m_pDevice->EndCommandBuffer(pCommandBuffer);

			m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
		}

		CE_DELETE(pShaderParamTable);
	}
}