#pragma once
#include "OpaqueContentRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE = "OpaqueColorTexture";
	const char* OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE = "OpaqueDepthTexture";

	const char* OpaqueContentRenderNode::INPUT_GBUFFER_NORMAL = "OpaqueInputGBufferNormal";
	const char* OpaqueContentRenderNode::INPUT_SHADOW_MAP = "OpaqueInputShadowMap";

	OpaqueContentRenderNode::OpaqueContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer)
	{
		m_inputResourceNames[INPUT_GBUFFER_NORMAL] = nullptr;
		m_inputResourceNames[INPUT_SHADOW_MAP] = nullptr;
	}

	void OpaqueContentRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		// Render pass object

		RenderPassAttachmentDescription colorDesc{};
		colorDesc.format = m_outputToSwapchain ? initInfo.swapSurfaceFormat : initInfo.colorFormat;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = m_outputToSwapchain ? EImageLayout::PresentSrc : EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = colorDesc.initialLayout;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		RenderPassAttachmentDescription depthDesc{};
		depthDesc.format = initInfo.depthFormat;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 1;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);
		passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Pipeline states

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

		m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, m_defaultPipelineStates.pVertexInputState);

		// Input assembly state

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, m_defaultPipelineStates.pInputAssemblyState);

		// Rasterization state

		PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
		rasterizationStateCreateInfo.enableDepthClamp = false;
		rasterizationStateCreateInfo.discardRasterizerResults = false;
		rasterizationStateCreateInfo.cullMode = ECullMode::Back;
		rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, m_defaultPipelineStates.pRasterizationState);

		// Depth stencil state

		PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
		depthStencilStateCreateInfo.enableDepthTest = true;
		depthStencilStateCreateInfo.enableDepthWrite = true;
		depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
		depthStencilStateCreateInfo.enableStencilTest = false;

		m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, m_defaultPipelineStates.pDepthStencilState);

		// Multisample state

		PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
		multisampleStateCreateInfo.enableSampleShading = false;
		multisampleStateCreateInfo.sampleCount = 1;

		m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, m_defaultPipelineStates.pMultisampleState);

		// Color blend state

		AttachmentColorBlendStateDescription attachmentNoBlendDesc{};
		attachmentNoBlendDesc.enableBlend = false;

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);

		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, m_defaultPipelineStates.pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		viewportStateCreateInfo.height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, m_defaultPipelineStates.pViewportState);
	}

	void OpaqueContentRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
		CreateMutableBuffers(initInfo);
	}

	void OpaqueContentRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		uint32_t height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		// Color output and shadow mark output

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
		texCreateInfo.textureWidth = width;
		texCreateInfo.textureHeight = height;
		texCreateInfo.format = ETextureFormat::RGBA8_SRGB;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		if (!m_outputToSwapchain)
		{
			for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
			{
				m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
			}
		}

		// Depth output

		texCreateInfo.format = initInfo.depthFormat;
		texCreateInfo.textureType = ETextureType::DepthAttachment;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pDepthOutput);

			m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
			m_graphResources[i]->Add(OUTPUT_DEPTH_TEXTURE, m_frameResources[i].m_pDepthOutput);
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_outputToSwapchain ? m_pSwapchainImages->at(i) : m_frameResources[i].m_pColorOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pDepthOutput);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;
			fbCreateInfo.renderToSwapchain = m_outputToSwapchain;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void OpaqueContentRenderNode::CreateMutableBuffers(const RenderNodeConfiguration& initInfo)
	{
		// Uniform buffers

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
		ubCreateInfo.maxSubAllocationCount = initInfo.maxDrawCall;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pTransformMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraMatrices);
		ubCreateInfo.maxSubAllocationCount = 1;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pLightSpaceTransformMatrix_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties);
		ubCreateInfo.maxSubAllocationCount = initInfo.maxDrawCall;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pMaterialNumericalProperties_UB);
		}
	}

	void OpaqueContentRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext)
	{
		auto pCameraTransform = (TransformComponent*)renderContext.pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)renderContext.pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}

		// Get resources

		auto& frameResources = m_frameResources[m_frameIndex];

		auto pGBufferNormalTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_NORMAL));
		auto pShadowMapTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_SHADOW_MAP));

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(cmdContext.pCommandPool);

		ShaderProgram* pShaderProgram = nullptr;
		EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;
		ShaderParameterTable shaderParamTable{};
		ESamplerAnisotropyLevel samplerAFLevel = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetTextureAnisotropyLevel();

		// Prepare uniform buffers

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();
		frameResources.m_pMaterialNumericalProperties_UB->ResetSubBufferAllocation();

		UBTransformMatrices ubTransformMatrices{};
		UBCameraMatrices ubCameraMatrices{};
		UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix{};
		UBCameraProperties ubCameraProperties{};
		UBMaterialNumericalProperties ubMaterialNumericalProperties{};

		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());
		ubCameraMatrices.projectionMatrix = projectionMat;
		ubCameraMatrices.viewMatrix = viewMat;
		frameResources.m_pCameraMatrices_UB->UpdateBufferData(&ubCameraMatrices);

		// TODO: remove these and get from light source
		Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
		Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
		Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
		Matrix4x4 lightSpaceMatrix = lightProjection * lightView;
		ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
		frameResources.m_pLightSpaceTransformMatrix_UB->UpdateBufferData(&ubLightSpaceTransformMatrix);

		ubCameraProperties.cameraPosition = cameraPos;
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		// Begin drawing

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);

		for (auto& entity : *renderContext.pOpaqueDrawList)
		{
			// Skip non-mesh entities

			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);
			if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			// Get vertex buffer

			auto pMesh = pMeshFilterComp->GetMesh();
			if (!pMesh)
			{
				continue;
			}
			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

			// Update per mesh uniforms

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
			SubUniformBuffer subTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices, &subTransformMatricesUB);

			// Draw submeshes
			auto pSubMeshes = pMesh->GetSubMeshes();
			uint32_t subMeshCount = pSubMeshes->size();
			for (uint32_t i = 0; i < subMeshCount; ++i)
			{
				// Skip transparent meshes
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);
				if (pMaterial->IsTransparent())
				{
					continue;
				}

				// Bind pipeline
				if (lastUsedShaderProgramType != pMaterial->GetShaderProgramType())
				{
					// TODO: implement missing pipeline building
					m_pDevice->BindGraphicsPipeline(GetGraphicsPipeline((uint32_t)pMaterial->GetShaderProgramType()), pCommandBuffer);
					pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
					lastUsedShaderProgramType = pMaterial->GetShaderProgramType();
				}

				// Update per submesh uniforms
				ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
				ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
				ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();
				SubUniformBuffer subMaterialNumericalPropertiesUB = frameResources.m_pMaterialNumericalProperties_UB->AllocateSubBuffer(sizeof(UBMaterialNumericalProperties));
				frameResources.m_pMaterialNumericalProperties_UB->UpdateBufferData(&ubMaterialNumericalProperties, &subMaterialNumericalPropertiesUB);

				// Update shader resources

				shaderParamTable.Clear();
				DEBUG_ASSERT_CE(pShaderProgram != nullptr);

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, &subTransformMatricesUB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_MATRICES), EDescriptorType::UniformBuffer, frameResources.m_pCameraMatrices_UB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pCameraProperties_UB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, frameResources.m_pLightSpaceTransformMatrix_UB);

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, &subMaterialNumericalPropertiesUB);

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferNormalTexture);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EDescriptorType::CombinedImageSampler, pShadowMapTexture);

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetTextureSampler(samplerAFLevel));
					shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
				if (pToneTexture)
				{
					if (!pToneTexture->HasSampler())
					{
						pToneTexture->SetSampler(m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None));
					}
					shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TONE_TEXTURE), EDescriptorType::CombinedImageSampler, pToneTexture);
				}

				m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);

				// Draw
				m_pDevice->DrawPrimitive(pSubMeshes->at(i).m_numIndices, pSubMeshes->at(i).m_baseIndex, pSubMeshes->at(i).m_baseVertex, pCommandBuffer);
			}
		}

		// End pass and submit

		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		frameResources.m_pTransformMatrices_UB->FlushToDevice();
		frameResources.m_pCameraMatrices_UB->FlushToDevice();
		frameResources.m_pLightSpaceTransformMatrix_UB->FlushToDevice();
		frameResources.m_pCameraProperties_UB->FlushToDevice();
		frameResources.m_pMaterialNumericalProperties_UB->FlushToDevice();

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void OpaqueContentRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);

		if (m_outputToSwapchain)
		{
			m_defaultPipelineStates.pViewportState->UpdateResolution(width, height);
		}
		else
		{
			m_defaultPipelineStates.pViewportState->UpdateResolution(width * m_configuration.renderScale, height * m_configuration.renderScale);
		}
	}

	void OpaqueContentRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;

		DestroyMutableBuffers();
		CreateMutableBuffers(m_configuration);
	}

	void OpaqueContentRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void OpaqueContentRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_SAFE_DELETE(m_frameResources[i].m_pColorOutput);
			CE_DELETE(m_frameResources[i].m_pDepthOutput);
		}
	}

	void OpaqueContentRenderNode::DestroyMutableBuffers()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pCameraMatrices_UB);
			CE_DELETE(m_frameResources[i].m_pCameraProperties_UB);
			CE_DELETE(m_frameResources[i].m_pLightSpaceTransformMatrix_UB);
			CE_DELETE(m_frameResources[i].m_pMaterialNumericalProperties_UB);
			CE_DELETE(m_frameResources[i].m_pTransformMatrices_UB);
		}
	}

	void OpaqueContentRenderNode::PrebuildGraphicsPipelines()
	{
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::AnimeStyle);
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::Basic);
	}
}