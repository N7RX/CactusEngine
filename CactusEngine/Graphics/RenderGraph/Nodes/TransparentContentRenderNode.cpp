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

	TransparentContentRenderNode::TransparentContentRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer)
		: RenderNode(pGraphResources, pRenderer)
	{
		m_inputResourceNames[INPUT_COLOR_TEXTURE] = nullptr;
		m_inputResourceNames[INPUT_BACKGROUND_DEPTH] = nullptr;
	}

	void TransparentContentRenderNode::SetupFunction(RenderGraphResource* pGraphResources)
	{
		uint32_t screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		// Color and depth texture

		Texture2DCreateInfo texCreateInfo{};
		texCreateInfo.generateMipmap = false;
		texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
		texCreateInfo.textureWidth = screenWidth;
		texCreateInfo.textureHeight = screenHeight;
		texCreateInfo.dataType = EDataType::Float32;
		texCreateInfo.format = ETextureFormat::RGBA32F;
		texCreateInfo.textureType = ETextureType::ColorAttachment;
		texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

		m_pDevice->CreateTexture2D(texCreateInfo, m_pColorOutput);

		texCreateInfo.format = ETextureFormat::Depth;
		texCreateInfo.textureType = ETextureType::DepthAttachment;

		m_pDevice->CreateTexture2D(texCreateInfo, m_pDepthOutput);

		pGraphResources->Add(OUTPUT_COLOR_TEXTURE, m_pColorOutput);
		pGraphResources->Add(OUTPUT_DEPTH_TEXTURE, m_pDepthOutput);

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
		depthDesc.index = 1;

		RenderPassCreateInfo passCreateInfo{};
		passCreateInfo.clearColor = Color4(1);
		passCreateInfo.clearDepth = 1.0f;
		passCreateInfo.clearStencil = 0;
		passCreateInfo.attachmentDescriptions.emplace_back(colorDesc);
		passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

		// Frame buffer

		FrameBufferCreateInfo fbCreateInfo{};
		fbCreateInfo.attachments.emplace_back(m_pColorOutput);
		fbCreateInfo.attachments.emplace_back(m_pDepthOutput);
		fbCreateInfo.framebufferWidth = screenWidth;
		fbCreateInfo.framebufferHeight = screenHeight;
		fbCreateInfo.pRenderPass = m_pRenderPassObject;

		m_pDevice->CreateFrameBuffer(fbCreateInfo, m_pFrameBuffer);

		// Uniform buffers

		uint32_t perSubmeshAllocation = m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan ? 4096 : 1;

		UniformBufferCreateInfo ubCreateInfo{};
		ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * perSubmeshAllocation;
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables);
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
		ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraProperties_UB);

		ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties) * perSubmeshAllocation;
		m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pMaterialNumericalProperties_UB);

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
		viewportStateCreateInfo.width = screenWidth;
		viewportStateCreateInfo.height = screenHeight;

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

		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::Basic_Transparent, pPipeline_0);
		m_graphicsPipelines.emplace(EBuiltInShaderProgramType::WaterBasic, pPipeline_1);
	}

	void TransparentContentRenderNode::RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext)
	{
		m_pTransformMatrices_UB->ResetSubBufferAllocation();
		m_pMaterialNumericalProperties_UB->ResetSubBufferAllocation();

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

		UBTransformMatrices ubTransformMatrices{};
		UBSystemVariables ubSystemVariables{};
		UBCameraProperties ubCameraProperties{};
		UBMaterialNumericalProperties ubMaterialNumericalProperties{};

		ubTransformMatrices.projectionMatrix = projectionMat;
		ubTransformMatrices.viewMatrix = viewMat;

		ubSystemVariables.timeInSec = Timer::Now();
		m_pSystemVariables_UB->UpdateBufferData(&ubSystemVariables);

		ubCameraProperties.cameraPosition = cameraPos;
		m_pCameraProperties_UB->UpdateBufferData(&ubCameraProperties);

		ShaderProgram* pShaderProgram = nullptr;
		EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;
		ShaderParameterTable* pShaderParamTable;
		CE_NEW(pShaderParamTable, ShaderParameterTable);

		m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer, pCommandBuffer);

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
				pSubTransformMatricesUB = m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
			}
			else
			{
				m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);
			}

			pShaderParamTable->Clear();

			uint32_t submeshCount = pMesh->GetSubmeshCount();
			auto subMeshes = pMesh->GetSubMeshes();

			m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

			for (uint32_t i = 0; i < submeshCount; ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

				if (!pMaterial->IsTransparent()) // TODO: use a list to record these parts instead of checking one-by-one
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
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
				}
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, m_pCameraProperties_UB);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, m_pSystemVariables_UB);

				ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
				ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
				ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();

				SubUniformBuffer* pSubMaterialNumericalPropertiesUB = nullptr;
				if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
				{
					pSubMaterialNumericalPropertiesUB = m_pMaterialNumericalProperties_UB->AllocateSubBuffer(sizeof(UBMaterialNumericalProperties));
					pSubMaterialNumericalPropertiesUB->UpdateSubBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubMaterialNumericalPropertiesUB);
				}
				else
				{
					m_pMaterialNumericalProperties_UB->UpdateBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, m_pMaterialNumericalProperties_UB);
				}

				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler,
					pGraphResources->Get(m_inputResourceNames.at(INPUT_BACKGROUND_DEPTH)));

				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
					pGraphResources->Get(m_inputResourceNames.at(INPUT_COLOR_TEXTURE)));

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pAlbedoTexture->SetSampler(m_pDevice->GetDefaultTextureSampler());
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
				if (pNoiseTexture)
				{
					pNoiseTexture->SetSampler(m_pDevice->GetDefaultTextureSampler());
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
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