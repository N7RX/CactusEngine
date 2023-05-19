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
		m_pVertexInputState_Empty(nullptr),
		m_pInputAssemblyState_Strip(nullptr),
		m_pRasterizationState_CullFront(nullptr),
		m_pColorBlendState_NoBlend(nullptr)
	{
		m_inputResourceNames[INPUT_GBUFFER_COLOR] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_NORMAL] = nullptr;
		m_inputResourceNames[INPUT_GBUFFER_POSITION] = nullptr;
		m_inputResourceNames[INPUT_DEPTH_TEXTURE] = nullptr;
	}

	void DeferredLightingRenderNode::CreateConstantResources(const RenderNodeConfiguration& initInfo)
	{
		// Render pass object

		RenderPassAttachmentDescription colorDesc{};
		colorDesc.format = m_outputToSwapchain ? initInfo.swapSurfaceFormat : initInfo.colorFormat;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = m_outputToSwapchain ? EImageLayout::PresentSrc : EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = colorDesc.initialLayout;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Pipeline states

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

		m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, m_defaultPipelineStates.pVertexInputState);

		PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo{};
		emptyVertexInputStateCreateInfo.bindingDescs = {};
		emptyVertexInputStateCreateInfo.attributeDescs = {};

		m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, m_pVertexInputState_Empty);

		// Input assembly states

		PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
		inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, m_pInputAssemblyState_Strip);

		inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;

		m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, m_defaultPipelineStates.pInputAssemblyState);

		// Rasterization states

		PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
		rasterizationStateCreateInfo.enableDepthClamp = false;
		rasterizationStateCreateInfo.discardRasterizerResults = false;
		rasterizationStateCreateInfo.cullMode = ECullMode::Front;
		rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, m_pRasterizationState_CullFront);

		rasterizationStateCreateInfo.cullMode = ECullMode::Back;

		m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, m_defaultPipelineStates.pRasterizationState);

		// Depth stencil state

		PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
		depthStencilStateCreateInfo.enableDepthTest = false;
		depthStencilStateCreateInfo.enableDepthWrite = false;
		depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
		depthStencilStateCreateInfo.enableStencilTest = false;

		m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, m_defaultPipelineStates.pDepthStencilState);

		// Multisample state

		PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
		multisampleStateCreateInfo.enableSampleShading = false;
		multisampleStateCreateInfo.sampleCount = 1;

		m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, m_defaultPipelineStates.pMultisampleState);

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

		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, m_defaultPipelineStates.pColorBlendState);

		AttachmentColorBlendStateDescription attachmentNoBlendDesc{};
		attachmentNoBlendDesc.enableBlend = false;

		PipelineColorBlendStateCreateInfo colorNoBlendStateCreateInfo{};
		colorNoBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		m_pDevice->CreatePipelineColorBlendState(colorNoBlendStateCreateInfo, m_pColorBlendState_NoBlend);

		// Viewport state

		PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		viewportStateCreateInfo.height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, m_defaultPipelineStates.pViewportState);
	}

	void DeferredLightingRenderNode::CreateMutableResources(const RenderNodeConfiguration& initInfo)
	{
		m_frameResources.resize(initInfo.framesInFlight);
		CreateMutableTextures(initInfo);
		CreateMutableBuffers(initInfo);
	}

	void DeferredLightingRenderNode::CreateMutableTextures(const RenderNodeConfiguration& initInfo)
	{
		uint32_t width = m_outputToSwapchain ? initInfo.width : initInfo.width * initInfo.renderScale;
		uint32_t height = m_outputToSwapchain ? initInfo.height : initInfo.height * initInfo.renderScale;

		// Color output

		if (!m_outputToSwapchain)
		{
			Texture2DCreateInfo texCreateInfo{};
			texCreateInfo.generateMipmap = false;
			texCreateInfo.pSampler = m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None);
			texCreateInfo.textureWidth = width;
			texCreateInfo.textureHeight = height;
			texCreateInfo.format = initInfo.colorFormat;
			texCreateInfo.textureType = ETextureType::ColorAttachment;
			texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

			for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
			{
				m_pDevice->CreateTexture2D(texCreateInfo, m_frameResources[i].m_pColorOutput);
				m_graphResources[i]->Add(OUTPUT_COLOR_TEXTURE, m_frameResources[i].m_pColorOutput);
			}
		}

		// Frame buffer

		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			FrameBufferCreateInfo fbCreateInfo{};
			fbCreateInfo.attachments.emplace_back(m_outputToSwapchain ? m_pSwapchainImages->at(i) : m_frameResources[i].m_pColorOutput);
			fbCreateInfo.framebufferWidth = width;
			fbCreateInfo.framebufferHeight = height;
			fbCreateInfo.pRenderPass = m_pRenderPassObject;
			fbCreateInfo.renderToSwapchain = m_outputToSwapchain;

			m_pDevice->CreateFrameBuffer(fbCreateInfo, m_frameResources[i].m_pFrameBuffer);
		}
	}

	void DeferredLightingRenderNode::CreateMutableBuffers(const RenderNodeConfiguration& initInfo)
	{
		// Uniform buffer

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

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pCameraProperties_UB);
		}

		ubCreateInfo.sizeInBytes = sizeof(UBLightSourceProperties);
		ubCreateInfo.maxSubAllocationCount = initInfo.maxDrawCall;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		for (uint32_t i = 0; i < initInfo.framesInFlight; ++i)
		{
			m_pDevice->CreateUniformBuffer(ubCreateInfo, m_frameResources[i].m_pLightSourceProperties_UB);
		}
	}

	void DeferredLightingRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext)
	{
		auto& frameResources = m_frameResources[m_frameIndex];

		GraphicsCommandBuffer* pCommandBuffer = m_pDevice->RequestCommandBuffer(cmdContext.pCommandPool);
		ShaderParameterTable shaderParamTable{};

		m_pDevice->BeginRenderPass(m_pRenderPassObject, frameResources.m_pFrameBuffer, pCommandBuffer);

		DirectionalLighting(pGraphResources, pCommandBuffer, shaderParamTable);
		RegularLighting(pGraphResources, renderContext, pCommandBuffer, shaderParamTable);

		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}

	void DeferredLightingRenderNode::DirectionalLighting(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable)
	{
		// Prepare shader
		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
		auto pGBufferColorTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_COLOR));
		shaderParamTable.AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GCOLOR_TEXTURE), EDescriptorType::CombinedImageSampler, pGBufferColorTexture);

		// Draw
		m_pDevice->BindGraphicsPipeline(GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::DeferredLighting_Directional), pCommandBuffer);
		m_pDevice->UpdateShaderParameter(pShaderProgram, &shaderParamTable, pCommandBuffer);
		m_pDevice->DrawFullScreenQuad(pCommandBuffer);
	}

	void DeferredLightingRenderNode::RegularLighting(RenderGraphResource* pGraphResources, const RenderContext& renderContext, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable)
	{
		auto pCameraTransform = (TransformComponent*)renderContext.pCamera->GetComponent(EComponentType::Transform);
		auto pCameraComp = (CameraComponent*)renderContext.pCamera->GetComponent(EComponentType::Camera);
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}

		auto& frameResources = m_frameResources[m_frameIndex];

		// Get input textures
		auto pGBufferColorTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_COLOR));
		auto pGBufferNormalTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_NORMAL));
		auto pGBufferPositionTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_GBUFFER_POSITION));
		auto pSceneDepthTexture = (Texture2D*)pGraphResources->Get(m_inputResourceNames.at(INPUT_DEPTH_TEXTURE));

		// Prepare uniform buffers

		frameResources.m_pTransformMatrices_UB->ResetSubBufferAllocation();
		frameResources.m_pLightSourceProperties_UB->ResetSubBufferAllocation();

		UBTransformMatrices ubTransformMatrices{};
		UBCameraMatrices ubCameraMatrices{};
		UBCameraProperties ubCameraProperties{};
		UBLightSourceProperties ubLightSourceProperties{};

		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		ubCameraMatrices.projectionMatrix = projectionMat;
		ubCameraMatrices.viewMatrix = viewMat;
		frameResources.m_pCameraMatrices_UB->UpdateBufferData(&ubCameraMatrices);

		ubCameraProperties.cameraPosition = cameraPos;
		frameResources.m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		// Bind pipeline and get shader

		m_pDevice->BindGraphicsPipeline(GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::DeferredLighting), pCommandBuffer);

		auto pShaderProgram = (m_pRenderer->GetRenderingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);

		// Draw light volume meshes
		for (auto& entity : *renderContext.pLightDrawList)
		{
			// Process only light components

			auto pLightComp = (LightComponent*)entity->GetComponent(EComponentType::Light);
			auto pTransformComp = (TransformComponent*)entity->GetComponent(EComponentType::Transform);

			auto lightProfile = pLightComp->GetProfile();
			if (lightProfile.sourceType != LightComponent::SourceType::Directional && !lightProfile.pVolumeMesh)
			{
				continue;
			}

			// Update uniforms

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			SubUniformBuffer subTransformMatricesUB = frameResources.m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			frameResources.m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices, &subTransformMatricesUB);

			ubLightSourceProperties.source = Vector4(pTransformComp->GetPosition(), (int)lightProfile.sourceType);
			ubLightSourceProperties.color = Color4(lightProfile.lightColor, 1.0f);
			ubLightSourceProperties.intensity = lightProfile.lightIntensity;
			ubLightSourceProperties.radius = lightProfile.radius;
			SubUniformBuffer subLightSourcePropertiesUB = frameResources.m_pLightSourceProperties_UB->AllocateSubBuffer(sizeof(UBLightSourceProperties));
			frameResources.m_pLightSourceProperties_UB->UpdateBufferData(&ubLightSourceProperties, &subLightSourcePropertiesUB);

			// Bind vertex buffer

			m_pDevice->SetVertexBuffer(lightProfile.pVolumeMesh->GetVertexBuffer(), pCommandBuffer);

			// Update shader resources

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

			// Draw submeshes
			auto pSubMeshes = lightProfile.pVolumeMesh->GetSubMeshes();
			uint32_t subMeshCount = pSubMeshes->size();
			for (uint32_t i = 0; i < subMeshCount; ++i)
			{
				m_pDevice->DrawPrimitive(pSubMeshes->at(i).m_numIndices, pSubMeshes->at(i).m_baseIndex, pSubMeshes->at(i).m_baseVertex, pCommandBuffer);
			}
		}

		// Flush local buffer data to device
		frameResources.m_pTransformMatrices_UB->FlushToDevice();
		frameResources.m_pCameraMatrices_UB->FlushToDevice();
		frameResources.m_pCameraProperties_UB->FlushToDevice();
		frameResources.m_pLightSourceProperties_UB->FlushToDevice();
	}

	void DeferredLightingRenderNode::UpdateResolution(uint32_t width, uint32_t height)
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

	void DeferredLightingRenderNode::UpdateMaxDrawCallCount(uint32_t count)
	{
		m_configuration.maxDrawCall = count;

		DestroyMutableBuffers();
		CreateMutableBuffers(m_configuration);
	}

	void DeferredLightingRenderNode::DestroyMutableResources()
	{
		m_frameResources.clear();
		m_frameResources.resize(0);
	}

	void DeferredLightingRenderNode::DestroyMutableTextures()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pFrameBuffer);
			CE_SAFE_DELETE(m_frameResources[i].m_pColorOutput);
		}
	}

	void DeferredLightingRenderNode::DestroyMutableBuffers()
	{
		for (uint32_t i = 0; i < m_frameResources.size(); ++i)
		{
			CE_DELETE(m_frameResources[i].m_pCameraMatrices_UB);
			CE_DELETE(m_frameResources[i].m_pCameraProperties_UB);
			CE_DELETE(m_frameResources[i].m_pLightSourceProperties_UB);
			CE_DELETE(m_frameResources[i].m_pTransformMatrices_UB);
		}
	}

	void DeferredLightingRenderNode::PrebuildGraphicsPipelines()
	{
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::DeferredLighting);
		GetGraphicsPipeline((uint32_t)EBuiltInShaderProgramType::DeferredLighting_Directional);
	}

	GraphicsPipelineObject* DeferredLightingRenderNode::GetGraphicsPipeline(uint32_t key)
	{
		if (m_graphicsPipelines.find(key) != m_graphicsPipelines.end())
		{
			return m_graphicsPipelines.at(key);
		}
		else
		{
			GraphicsPipelineCreateInfo pipelineCreateInfo{};

			switch (key)
			{
				case (uint32_t)EBuiltInShaderProgramType::DeferredLighting:
				{
					pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting);
					pipelineCreateInfo.pVertexInputState = m_defaultPipelineStates.pVertexInputState;
					pipelineCreateInfo.pInputAssemblyState = m_defaultPipelineStates.pInputAssemblyState;
					pipelineCreateInfo.pColorBlendState = m_defaultPipelineStates.pColorBlendState;
					pipelineCreateInfo.pRasterizationState = m_pRasterizationState_CullFront;
					pipelineCreateInfo.pDepthStencilState = m_defaultPipelineStates.pDepthStencilState;
					pipelineCreateInfo.pMultisampleState = m_defaultPipelineStates.pMultisampleState;
					pipelineCreateInfo.pViewportState = m_defaultPipelineStates.pViewportState;
					pipelineCreateInfo.pRenderPass = m_pRenderPassObject;
					break;
				}
				case (uint32_t)EBuiltInShaderProgramType::DeferredLighting_Directional:
				{
					pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DeferredLighting_Directional);
					pipelineCreateInfo.pVertexInputState = m_pVertexInputState_Empty;
					pipelineCreateInfo.pInputAssemblyState = m_pInputAssemblyState_Strip;
					pipelineCreateInfo.pColorBlendState = m_pColorBlendState_NoBlend;
					pipelineCreateInfo.pRasterizationState = m_defaultPipelineStates.pRasterizationState;
					pipelineCreateInfo.pDepthStencilState = m_defaultPipelineStates.pDepthStencilState;
					pipelineCreateInfo.pMultisampleState = m_defaultPipelineStates.pMultisampleState;
					pipelineCreateInfo.pViewportState = m_defaultPipelineStates.pViewportState;
					pipelineCreateInfo.pRenderPass = m_pRenderPassObject;
					break;
				}
				default:
				{
					LOG_ERROR("Requested graphics pipeline cannot be found and cannot be created.");
					return nullptr;
				}
			}

			GraphicsPipelineObject* pPipeline = nullptr;
			m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);
			m_graphicsPipelines.emplace(key, pPipeline);

			return pPipeline;
		}
	}
}