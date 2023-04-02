#pragma once
#include "ShadowMapRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE = "ShadowMapDepthTexture";

	ShadowMapRenderNode::ShadowMapRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject(nullptr)
	{

	}

	void ShadowMapRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		DEBUG_ASSERT_MESSAGE_CE(!m_outputToSwapchain, "Shadow map render node cannot output directly to swapchain.");

		// Render pass object

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
		depthDesc.index = 0;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Pipeline object

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

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};

		PipelineColorBlendState* pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
		viewportStateCreateInfo.height = SHADOW_MAP_RESOLUTION * initInfo.renderScale;

		PipelineViewportState* pViewportState = nullptr;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

		// Pipeline creation

		GraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		pipelineCreateInfo.pVertexInputState = pVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

		GraphicsPipelineObject* pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

		m_graphicsPipelines.emplace((uint32_t)EBuiltInShaderProgramType::ShadowMap, pPipeline);
	}

	void ShadowMapRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
		CreateMutableBuffers(initInfo);
	}

	void ShadowMapRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		// Depth texture

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
		texCreateInfo.textureWidth = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
		texCreateInfo.textureHeight = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
		texCreateInfo.format = initInfo.depthFormat;
		texCreateInfo.textureType = ETextureType::DepthAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pDepthOutput);
			m_graphResources[i]->Add(OUTPUT_DEPTH_TEXTURE, m_frameResources[i].m_pDepthOutput);
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pDepthOutput);
			fbCreateInfo.framebufferWidth = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
			fbCreateInfo.framebufferHeight = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void ShadowMapRenderNode::CreateMutableBuffers(const RenderNodeConfiguration& initInfo)
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

		ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
		ubCreateInfo.maxSubAllocationCount = 1;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pLightSpaceTransformMatrix_UB);
		}
	}

	void ShadowMapRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		ShaderParameterTable shaderParamTable{};

		// Prepare uniform buffers

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();

		UBTransformMatrices ubTransformMatrices{};
		UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix{};

		Matrix4x4 lightProjection;
		Matrix4x4 lightView;
		// TODO: resolve this workaround and get from light source
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			Vector3 lightDir(0.0f, 0.8660254f * 16, -0.5f * 16);
			lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -30.0f, 30.0f);
			lightView = glm::lookAt(lightDir, Vector3(0), UP);
		}
		else
		{
			Vector3 lightDir(0.0f, 0.8660254f, -0.5f);
			lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
		}
		Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

		ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
		frameResources.m_pLightSpaceTransformMatrix_UB->UpdateBufferData(&ubLightSpaceTransformMatrix);

		// Bind pipeline and begin draw

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at((uint32_t)EBuiltInShaderProgramType::ShadowMap), pCommandBuffer);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);
			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);
			if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			// Bind vertext buffer
			auto pMesh = pMeshFilterComp->GetMesh();
			if (!pMesh)
			{
				continue;
			}
			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);	

			// Update uniforms
			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			SubUniformBuffer subTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices, &subTransformMatricesUB);

			// Draw submeshes
			auto pSubMeshes = pMesh->GetSubMeshes();
			uint32_t subMeshCount = pSubMeshes->size();
			for (uint32_t i = 0; i < subMeshCount; ++i)
			{
				// Skip transparent sub mesh
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);
				if (pMaterial->IsTransparent())
				{
					continue;
				}

				// Update shader resources

				shaderParamTable.Clear();

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, &subTransformMatricesUB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, frameResources.m_pLightSpaceTransformMatrix_UB);

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None));
					shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
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
		frameResources.m_pLightSpaceTransformMatrix_UB->FlushToDevice();

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);

		// Special reset case
		if (m_eGraphicsDeviceType == EGraphicsAPIType::OpenGL)
		{
			m_pDevice->ResizeViewPort(m_configuration.width, m_configuration.height);
		}
	}

	void ShadowMapRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);
	}

	void ShadowMapRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;

		if (m_eGraphicsDeviceType != EGraphicsAPIType::OpenGL)
		{
			DestroyMutableBuffers();
			CreateMutableBuffers(m_configuration);
		}
	}

	void ShadowMapRenderNode::UpdateFramesInFlight(uint32_t framesInFlight)
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

	void ShadowMapRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void ShadowMapRenderNode::DestroyConstantResources()
	{
		CE_DELETE(m_pRenderPassObject);
		DestroyGraphicsPipelines();
	}

	void ShadowMapRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_DELETE(m_frameResources[i].m_pDepthOutput);
		}
	}

	void ShadowMapRenderNode::DestroyMutableBuffers()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pLightSpaceTransformMatrix_UB);
			CE_DELETE(m_frameResources[i].m_pTransformMatrices_UB);
		}
	}
}