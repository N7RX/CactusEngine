#pragma once
#include "ShadowMapRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE = "ShadowMapDepthTexture";

	ShadowMapRenderNode::ShadowMapRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer)
	{
		// Reserve by 512 KB; this should be enough for most cases
		// Assuming each mesh takes up 256 bytes, this would be enough for 2048 meshes until a new region is allocated
		CE_NEW(m_pUniformBufferAllocator, UniformBufferConcurrentAllocator, pRenderer->GetBufferManager(), 512 * 1024);
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

		// Pipeline states

		// Vertex input state

		PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = GetDefaultVertexInputStateCreateInfo();

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

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};

		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, m_defaultPipelineStates.pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = SHADOW_MAP_RESOLUTION * initInfo.renderScale;
		viewportStateCreateInfo.height = SHADOW_MAP_RESOLUTION * initInfo.renderScale;

		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, m_defaultPipelineStates.pViewportState);
	}

	void ShadowMapRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
	}

	void ShadowMapRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
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

	void ShadowMapRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext)
	{
		m_pUniformBufferAllocator->ResetReservedRegion();
		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(cmdContext.pCommandPool);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		ShaderParameterTable shaderParamTable{};

		// Prepare uniform buffer

		UniformBuffer lightSpaceTransformMatrix_UB = m_pUniformBufferAllocator->GetUniformBuffer(sizeof(UBLightSpaceTransformMatrix));

		UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix{};

		Matrix4x4 lightProjection;
		Matrix4x4 lightView;

		// TODO: remove this workaround and get from light source
		Vector3 lightDir(0.0f, 0.8660254f * 16, -0.5f * 16);
		lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -30.0f, 30.0f);
		lightView = glm::lookAt(lightDir, Vector3(0), UP);

		Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

		ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
		lightSpaceTransformMatrix_UB.UpdateBufferData(&ubLightSpaceTransformMatrix);

		// Bind pipeline and begin draw

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::ShadowMap), pCommandBuffer);

		for (auto& entity : *renderContext.pOpaqueDrawList)
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

			// Update uniform buffer

			UniformBuffer transformMatrices_UB = m_pUniformBufferAllocator->GetUniformBuffer(sizeof(UBTransformMatrices));

			UBTransformMatrices ubTransformMatrices{};

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			transformMatrices_UB.UpdateBufferData(&ubTransformMatrices);

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

				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, &transformMatrices_UB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, &lightSpaceTransformMatrix_UB);

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

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void ShadowMapRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		// TODO: adjust shadow resoltuion independently
		
		//m_configuration.width = width;
		//m_configuration.height = height;

		//DestroyMutableTextures();
		//CreateMutableTextures(m_configuration);
	}

	void ShadowMapRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_DELETE(m_frameResources[i].m_pDepthOutput);
		}
	}

	void ShadowMapRenderNode::PrebuildGraphicsPipelines()
	{
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::ShadowMap);
	}
}