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

	DeferredLightingRenderNode::DeferredLightingRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer)
		: RenderNode(graphResources, pRenderer),
		m_pRenderPassObject(nullptr)
	{
		m_inputResourceNames[INPUT_GBUFFER_COLOR] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_NORMAL] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
		m_inputResourceNames[INPUT_DEPTH_TEXTURE] = nullptr;
	}

	void DeferredLightingRenderNode::SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight)
	{
		m_frameResources.resize(framesInFlight);

		// Color output

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = width;
		texCreateInfo.textureHeight = height;
		texCreateInfo.dataType = EDataType::UByte;
		texCreateInfo.format = ETextureFormat::RGBA8_SRGB;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
			m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
		}

		// Render pass object

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

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Frame buffer

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_frameResources[i].m_pColorOutput);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}

		// Uniform buffer

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
		ubCreateInfo.maxSubAllocationCount = maxDrawCall;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pTransformMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraMatrices);
		ubCreateInfo.maxSubAllocationCount = 1;
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraMatrices_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBLightSourceProperties);
		ubCreateInfo.maxSubAllocationCount = maxDrawCall;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pLightSourceProperties_UB);
		}

		// Pipeline object

		// Vertex input states

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

		PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo{};
		emptyVertexInputStateCreateInfo.bindingDescs = {};
		emptyVertexInputStateCreateInfo.attributeDescs = {};

		PipelineVertexInputState* pEmptyVertexInputState = nullptr;
		m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

		// Input assembly states

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		PipelineInputAssemblyState* pInputAssemblyState_Strip = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_Strip);

		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;

		PipelineInputAssemblyState* pInputAssemblyState_List = nullptr;
		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_List);

		// Rasterization states

		PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
		rasterizationStateCreateInfo.enableDepthClamp = false;
		rasterizationStateCreateInfo.discardRasterizerResults = false;
		rasterizationStateCreateInfo.cullMode = ECullMode::Front;
		rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

		PipelineRasterizationState* pCullFrontRasterizationState = nullptr;
		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pCullFrontRasterizationState);

		rasterizationStateCreateInfo.cullMode = ECullMode::Back;

		PipelineRasterizationState* pCullBackRasterizationState = nullptr;
		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pCullBackRasterizationState);

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

		AttachmentColorBlendStateDescription attachmentBlendDesc{};
		attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
		attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
		attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::One;
		attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::One;
		attachmentBlendDesc.enableBlend = true;

		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		PipelineColorBlendState* pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		AttachmentColorBlendStateDescription attachmentNoBlendDesc{};
		attachmentNoBlendDesc.enableBlend = false;

		PipelineColorBlendStateCreateInfo colorNoBlendStateCreateInfo{};
		colorNoBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		PipelineColorBlendState* pColorNoBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorNoBlendStateCreateInfo, pColorNoBlendState);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = width;
		viewportStateCreateInfo.height = height;

		PipelineViewportState* pViewportState = nullptr;
		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

		// Pipeline creation

		GraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);
		pipelineCreateInfo.pVertexInputState = pVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		pipelineCreateInfo.pColorBlendState = pColorBlendState;
		pipelineCreateInfo.pRasterizationState = pCullFrontRasterizationState;
		pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
		pipelineCreateInfo.pMultisampleState = pMultisampleState;
		pipelineCreateInfo.pViewportState = pViewportState;
		pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

		GraphicsPipelineObject* pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DeferredLighting, pPipeline);

		pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
		pipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		pipelineCreateInfo.pColorBlendState = pColorNoBlendState;
		pipelineCreateInfo.pRasterizationState = pCullBackRasterizationState;

		GraphicsPipelineObject* pDirPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pDirPipeline);

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::DeferredLighting_Directional, pDirPipeline);
	}

	void DeferredLightingRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		auto& frameResources = m_frameResources[m_frameIndex];

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();
		frameResources.m_pLightSourceProperties_UB->ResetSubBufferAllocation();

		auto pGBufferColorTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_COLOR));
		auto pGBufferNormalTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_NORMAL));
		auto pGBufferPositionTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION));
		auto pSceneDepthTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_DEPTH_TEXTURE));

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);

		// Directional light pass (pass through)

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DeferredLighting_Directional), pCommandBuffer);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
		ShaderParameterTable shaderParamTable;

		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GCOLOR_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferColorTexture);

		m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);

		shaderParamTable.Clear();

		// Other light sources pass

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

		m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::DeferredLighting), pCommandBuffer);

		UBTransformMatrices ubTransformMatrices{};
		UBCameraMatrices ubCameraMatrices{};
		UBCameraProperties ubCameraProperties{};
		UBLightSourceProperties ubLightSourceProperties{};

		pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);

		ubCameraMatrices.projectionMatrix = projectionMat;
		ubCameraMatrices.viewMatrix = viewMat;
		frameResources.m_pCameraMatrices_UB->UpdateBufferData(&ubCameraMatrices);

		ubCameraProperties.cameraPosition = cameraPos;
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		for (auto& entity : *pRenderContext->pDrawList)
		{
			auto pLightComp = (LightComponent*)entity->GetComponent(EComponentType::Light);
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);

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

			SubUniformBuffer subTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices, &subTransformMatricesUB);

			ubLightSourceProperties.source = Vector4(pTransformComp->GetPosition(), (int)lightProfile.sourceType);
			ubLightSourceProperties.color = Color4(lightProfile.lightColor, 1.0f);
			ubLightSourceProperties.intensity = lightProfile.lightIntensity;
			ubLightSourceProperties.radius = lightProfile.radius;

			SubUniformBuffer subLightSourcePropertiesUB = frameResources.m_pLightSourceProperties_UB->AllocateSubBuffer(sizeof(UBLightSourceProperties));
			frameResources.m_pLightSourceProperties_UB->UpdateBufferData(&ubLightSourceProperties, &subLightSourcePropertiesUB);

			uint32_t submeshCount = lightProfile.pVolumeMesh->GetSubmeshCount();
			auto subMeshes = lightProfile.pVolumeMesh->GetSubMeshes();

			m_pDevice->SetVertexBuffer(lightProfile.pVolumeMesh->GetVertexBuffer(), pCommandBuffer);

			for (uint32_t i = 0; i < submeshCount; ++i)
			{
				shaderParamTable.Clear();
				
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_MATRICES), EDescriptorType::UniformBuffer, frameResources.m_pCameraMatrices_UB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, frameResources.m_pCameraProperties_UB);
				
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, &subTransformMatricesUB);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSOURCE_PROPERTIES), EDescriptorType::SubUniformBuffer, &subLightSourcePropertiesUB);
				
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GCOLOR_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferColorTexture);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferNormalTexture);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferPositionTexture);
				shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, pSceneDepthTexture);

				m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);
				m_pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
			}
		}

		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		frameResources.m_pTransformMatrices_UB->FlushToDevice();
		frameResources.m_pCameraMatrices_UB->FlushToDevice();
		frameResources.m_pCameraProperties_UB->FlushToDevice();
		frameResources.m_pLightSourceProperties_UB->FlushToDevice();

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void DeferredLightingRenderNode::UpdateResolution(uint32_t width, uint32_t height)
	{

	}

	void DeferredLightingRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{

	}

	void DeferredLightingRenderNode::UpdateFramesInFlight(uint32_t framesInFlight)
	{

	}
}