#pragma once
#include "TransparentContentRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"
#include "Timer.h"

namespace Engine
{
	const char* TransparentContentRenderNode::OUTPUT_COLOR_TEXTURE = "TransparencyColorTexture";
	const char* TransparentContentRenderNode::OUTPUT_DEPTH_TEXTURE = "TransparencyDepthTexture";

	const char* TransparentContentRenderNode::INPUT_COLOR_TEXTURE = "TransparencyBackgroundColorTexture";
	const char* TransparentContentRenderNode::INPUT_BACKGROUND_DEPTH = "TransparencyBackgroundDepthTexture";

	TransparentContentRenderNode::TransparentContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject(nullptr)
	{
		m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_BACKGROUND_DEPTH] = nullptr;
	}

	void TransparentContentRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		DEBUG_ASSERT_MESSAGE_CE(!m_outputToSwapchain, "Transparent content render node cannot output directly to swapchain.");

		// Render pass object

		RenderPassAttachmentDescription colorDesc{};
		colorDesc.format = initInfo.colorFormat;
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

		PipelineColorBlendState* pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = initInfo.width * initInfo.renderScale;
		viewportStateCreateInfo.height = initInfo.height * initInfo.renderScale;

		PipelineViewportState* pViewportState = nullptr;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

		// Pipeline creation

		GraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic_Transparent);
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

		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::WaterBasic);

		GraphicsPipelineObject* pPipeline_1 = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline_1);

		m_graphicsPipelines.emplace((uint32_t)EBuiltInShaderProgramType::Basic_Transparent, pPipeline_0);
		m_graphicsPipelines.emplace((uint32_t)EBuiltInShaderProgramType::WaterBasic, pPipeline_1);
	}

	void TransparentContentRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
		CreateMutableBuffers(initInfo);
	}

	void TransparentContentRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = initInfo.width * initInfo.renderScale;
		uint32_t height = initInfo.height * initInfo.renderScale;

		// Color and depth texture

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
		texCreateInfo.textureWidth = width;
		texCreateInfo.textureHeight = height;
		texCreateInfo.format = ETextureFormat::RGBA8_SRGB;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
			m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
		}

		texCreateInfo.format = initInfo.depthFormat;
		texCreateInfo.textureType = ETextureType::DepthAttachment;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pDepthOutput);
			m_graphResources[i]->Add(OUTPUT_DEPTH_TEXTURE, m_frameResources[i].m_pDepthOutput);
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pColorOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pDepthOutput);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void TransparentContentRenderNode::CreateMutableBuffers(const RenderNodeConfiguration& initInfo)
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

		ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables);
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pSystemVariables_UB);
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

	void TransparentContentRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto pCameraTransform = (TransformComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)pRenderContext->pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}

		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		ShaderProgram* pShaderProgram = nullptr;
		EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;
		ShaderParameterTable shaderParamTable;
		ESamplerAnisotropyLevel samplerAFLevel = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetTextureAnisotropyLevel();

		// Prepare uniform buffers

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();
		frameResources.m_pMaterialNumericalProperties_UB->ResetSubBufferAllocation();

		UBTransformMatrices ubTransformMatrices{};
		UBCameraMatrices ubCameraMatrices{};
		UBSystemVariables ubSystemVariables{};
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

		ubSystemVariables.timeInSec = Timer::Now();
		frameResources.m_pSystemVariables_UB->UpdateBufferData(&ubSystemVariables);

		ubCameraProperties.cameraPosition = cameraPos;
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		// Begin draw

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);
			if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			// Bind vertex buffer
			auto pMesh = pMeshFilterComp->GetMesh();
			if (!pMesh)
			{
				continue;
			}
			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

			// Update uniforms
			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
			SubUniformBuffer subTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices, &subTransformMatricesUB);

			// Draw submeshes
			auto pSubMeshes = pMesh->GetSubMeshes();
			uint32_t subMeshCount = pSubMeshes->size();
			for (uint32_t i = 0; i < subMeshCount; ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);
				if (!pMaterial->IsTransparent()) // TODO: use a list to record transparent objects instead of checking one-by-one
				{
					continue;
				}

				// Bind pipeline
				if (lastUsedShaderProgramType != pMaterial->GetShaderProgramType())
				{
					m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at((uint32_t)pMaterial->GetShaderProgramType()), pCommandBuffer);
					pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
					lastUsedShaderProgramType = pMaterial->GetShaderProgramType();
				}

				// Update uniform
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
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, frameResources.m_pSystemVariables_UB);
				
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, &subMaterialNumericalPropertiesUB);

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler,
					pGraphResources->Get(m_inputResourceNames.at(INPUT_BACKGROUND_DEPTH)));
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
					pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetTextureSampler(samplerAFLevel));
					shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
				if (pNoiseTexture)
				{
					pNoiseTexture->SetSampler(m_pDevice->GetTextureSampler(samplerAFLevel));
					shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
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
		frameResources.m_pSystemVariables_UB->FlushToDevice();
		frameResources.m_pCameraProperties_UB->FlushToDevice();
		frameResources.m_pMaterialNumericalProperties_UB->FlushToDevice();

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void TransparentContentRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);
	}

	void TransparentContentRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;

		if (m_eGraphicsDeviceType != EGraphicsAPIType::OpenGL)
		{
			DestroyMutableBuffers();
			CreateMutableBuffers(m_configuration);
		}
	}

	void TransparentContentRenderNode::UpdateFramesInFlight(uint32_t framesInFlight)
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

	void TransparentContentRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void TransparentContentRenderNode::DestroyConstantResources()
	{
		CE_DELETE(m_pRenderPassObject);
		DestroyGraphicsPipelines();
	}

	void TransparentContentRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_DELETE(m_frameResources[i].m_pColorOutput);
			CE_DELETE(m_frameResources[i].m_pDepthOutput);
		}
	}

	void TransparentContentRenderNode::DestroyMutableBuffers()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pCameraMatrices_UB);
			CE_DELETE(m_frameResources[i].m_pCameraProperties_UB);
			CE_DELETE(m_frameResources[i].m_pSystemVariables_UB);
			CE_DELETE(m_frameResources[i].m_pMaterialNumericalProperties_UB);
			CE_DELETE(m_frameResources[i].m_pTransformMatrices_UB);
		}
	}
}