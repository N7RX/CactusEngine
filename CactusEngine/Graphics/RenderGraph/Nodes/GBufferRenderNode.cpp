#pragma once
#include "GBufferRenderNode.h"
#include "RenderingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

namespace Engine
{
	const char* GBufferRenderNode::OUTPUT_NORMAL_GBUFFER = "NormalGBufferTexture";
	const char* GBufferRenderNode::OUTPUT_POSITION_GBUFFER = "PositionGBufferTexture";

	const ETextureFormat GBUFFER_COLOR_FORMAT = ETextureFormat::RGBA32F;

	GBufferRenderNode::GBufferRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer)
	{
		// Reserve by 1 MB; this should be enough for most cases
		// Assuming each mesh takes up 512 bytes, this would be enough for 2048 meshes until a new region is allocated
		CE_NEW(m_pUniformBufferAllocator, UniformBufferConcurrentAllocator, pRenderer->GetBufferManager(), 1 * 1024 * 1024);
	}

	void GBufferRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		DEBUG_ASSERT_MESSAGE_CE(!m_outputToSwapchain, "GBuffer render node cannot output directly to swapchain.");

		// Render pass object

		RenderPassAttachmentDescription normalDesc{};
		normalDesc.format = GBUFFER_COLOR_FORMAT;
		normalDesc.sampleCount = 1;
		normalDesc.loadOp = EAttachmentOperation::Clear;
		normalDesc.storeOp = EAttachmentOperation::Store;
		normalDesc.stencilLoadOp = EAttachmentOperation::None;
		normalDesc.stencilStoreOp = EAttachmentOperation::None;
		normalDesc.initialLayout = EImageLayout::ShaderReadOnly;
		normalDesc.usageLayout = EImageLayout::ColorAttachment;
		normalDesc.finalLayout = EImageLayout::ShaderReadOnly;
		normalDesc.type = EAttachmentType::Color;
		normalDesc.index = 0;

		RenderPassAttachmentDescription positionDesc{};
		positionDesc.format = GBUFFER_COLOR_FORMAT;
		positionDesc.sampleCount = 1;
		positionDesc.loadOp = EAttachmentOperation::Clear;
		positionDesc.storeOp = EAttachmentOperation::Store;
		positionDesc.stencilLoadOp = EAttachmentOperation::None;
		positionDesc.stencilStoreOp = EAttachmentOperation::None;
		positionDesc.initialLayout = EImageLayout::ShaderReadOnly;
		positionDesc.usageLayout = EImageLayout::ColorAttachment;
		positionDesc.finalLayout = EImageLayout::ShaderReadOnly;
		positionDesc.type = EAttachmentType::Color;
		positionDesc.index = 1;

		RenderPassAttachmentDescription depthDesc{};
		depthDesc.format = initInfo.depthFormat;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 2;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(normalDesc);
		passCreateInfo.attachmentDescriptions.emplace_back(positionDesc);
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
		colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);

		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, m_defaultPipelineStates.pColorBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = initInfo.width * initInfo.renderScale;
		viewportStateCreateInfo.height = initInfo.height * initInfo.renderScale;

		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, m_defaultPipelineStates.pViewportState);
	}

	void GBufferRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
	}

	void GBufferRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void GBufferRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = initInfo.width * initInfo.renderScale;
		uint32_t height = initInfo.height * initInfo.renderScale;

		// GBuffer color textures

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
		texCreateInfo.textureWidth = width;
		texCreateInfo.textureHeight = height;
		texCreateInfo.format = GBUFFER_COLOR_FORMAT;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pNormalOutput);
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pPositionOutput);

			m_graphResources[i]->Add(OUTPUT_NORMAL_GBUFFER, m_frameResources[i].m_pNormalOutput);
			m_graphResources[i]->Add(OUTPUT_POSITION_GBUFFER, m_frameResources[i].m_pPositionOutput);
		}

		// Depth attachment

		texCreateInfo.format = initInfo.depthFormat;
		texCreateInfo.textureType = ETextureType::DepthAttachment;
		texCreateInfo.initialLayout = EImageLayout::DepthStencilAttachment;

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pDepthBuffer);
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pNormalOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pPositionOutput);
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pDepthBuffer);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void GBufferRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext)
	{
		auto pCameraTransform = (TransformComponent*)renderContext.pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)renderContext.pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}

		m_pUniformBufferAllocator->ResetReservedRegion();
		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(cmdContext.pCommandPool);
		// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GBuffer);
		ShaderParameterTable shaderParamTable{};

		// Prepare uniform buffers

		UniformBuffer cameraMatrices_UB = m_pUniformBufferAllocator->GetUniformBuffer(sizeof(UBCameraMatrices));

		UBCameraMatrices ubCameraMatrices{};

		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		ubCameraMatrices.projectionMatrix = projectionMat;
		ubCameraMatrices.viewMatrix = viewMat;
		cameraMatrices_UB.UpdateBufferData(&ubCameraMatrices);

		// Bind pipeline and draw
		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);
		m_pDevice->BindGraphicsPipeline(GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::GBuffer), pCommandBuffer);

		for (auto& entity : *renderContext.pOpaqueDrawList)
		{
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);
			auto pMeshFilterComp = (MeshFilterComponent*)entity->GetComponent(EComponentType::MeshFilter);
			if (!pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

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
			ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
			transformMatrices_UB.UpdateBufferData(&ubTransformMatrices);

			// Update shader resources

			shaderParamTable.Clear();

			shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, &transformMatrices_UB);
			shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_MATRICES), EDescriptorType::UniformBuffer, &cameraMatrices_UB);

			m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);

			// Draw only opaque submeshes

			auto pMaterialComp = (MaterialComponent*)entity->GetComponent(EComponentType::Material);

			auto pSubMeshes = pMesh->GetSubMeshes();
			uint32_t subMeshCount = pSubMeshes->size();
			for (uint32_t i = 0; i < subMeshCount; ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);
				if (pMaterial->IsTransparent())
				{
					continue;
				}

				m_pDevice->DrawPrimitive(pSubMeshes->at(i).m_numIndices, pSubMeshes->at(i).m_baseIndex, pSubMeshes->at(i).m_baseVertex, pCommandBuffer);
			}
		}

		// End pass and submit

		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void GBufferRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{
		m_configuration.width = width;
		m_configuration.height = height;

		DestroyMutableTextures();
		CreateMutableTextures(m_configuration);

		m_defaultPipelineStates.pViewportState->UpdateResolution(width * m_configuration.renderScale, height * m_configuration.renderScale);
	}

	void GBufferRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_DELETE(m_frameResources[i].m_pDepthBuffer);
			CE_DELETE(m_frameResources[i].m_pNormalOutput);
			CE_DELETE(m_frameResources[i].m_pPositionOutput);
		}
	}

	void GBufferRenderNode::PrebuildGraphicsPipelines()
	{
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::GBuffer);
	}
}