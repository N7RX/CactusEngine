#pragma once
#include "ShadowMapRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE = "ShadowMapDepthTexture";

	ShadowMapRenderNode::ShadowMapRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer)
		: RenderNode(pGraphResources, pRenderer)
	{

	}

	void ShadowMapRenderNode::SetupFunction(RenderGraphResource* pGraphResources)
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		// Depth texture

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = SHADOW_MAP_RESOLUTION;
		texCreateInfo.textureHeight = SHADOW_MAP_RESOLUTION;
		texCreateInfo.dataType = EDataType::Float32;
		texCreateInfo.format = ETextureFormat::Depth;
		texCreateInfo.textureType = ETextureType::DepthAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		m_pDevice->CreateTexture2D(texCreateInfo, m_pDepthOutput);

		pGraphResources->Add(OUTPUT_DEPTH_TEXTURE, m_pDepthOutput);

		// Render pass object

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
		depthDesc.index = 0;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Frame buffer

		FrameBufferCreateInfo fbCreateInfo{};
		fbCreateInfo.attachments.emplace_back(m_pDepthOutput);
		fbCreateInfo.framebufferWidth = SHADOW_MAP_RESOLUTION;
		fbCreateInfo.framebufferHeight = SHADOW_MAP_RESOLUTION;
		fbCreateInfo.pRenderPass = m_pRenderPassObject;

		m_pDevice->CreateFrameBuffer(fbCreateInfo, m_pFrameBuffer);

		// Uniform buffers

		uint32_t perSubmeshAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 4096 : 1;

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * perSubmeshAllocation;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pLightSpaceTransformMatrix_UB);

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
		viewportStateCreateInfo.width = SHADOW_MAP_RESOLUTION;
		viewportStateCreateInfo.height = SHADOW_MAP_RESOLUTION;

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

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::ShadowMap, pPipeline);
	}

	void ShadowMapRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		m_pTransformMatrices_UB->ResetSubBufferAllocation();

		Matrix4x4 lightProjection;
		Matrix4x4 lightView;
		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan) // TODO: resolve this workaround for Vulkan
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

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer, pCommandBuffer);

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::ShadowMap), pCommandBuffer);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		ShaderParameterTable* pShaderParamTable;
		CE_NEW(pShaderParamTable, ShaderParameterTable);

		UBTransformMatrices ubTransformMatrices{};
		UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix{};

		ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
		m_pLightSpaceTransformMatrix_UB->UpdateBufferData(&ubLightSpaceTransformMatrix);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);
			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);

			if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			auto pMesh = pMeshFilterComp->GetMesh();

			if (!pMesh)
			{
				continue;
			}

			pShaderParamTable->Clear();

			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			SubUniformBuffer* pSubTransformMatricesUB = nullptr;
			if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
			{
				pSubTransformMatricesUB = m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
			}
			else
			{
				m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);
			}

			auto subMeshes = pMesh->GetSubMeshes();
			for (uint32_t i = 0; i < subMeshes->size(); ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

				if (pMaterial->IsTransparent())
				{
					continue;
				}

				pShaderParamTable->Clear();

				if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
				}
				else
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
				}
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, m_pLightSpaceTransformMatrix_UB);

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetDefaultTextureSampler());
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
				m_pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);

				if (m_eGraphicsDeviceType != EGraphicsAPIType::Vulkan)
				{
					pShaderProgram->Reset();
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
		else
		{
			m_pDevice->ResizeViewPort(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight());
		}

		CE_DELETE(pShaderParamTable);
	}
}