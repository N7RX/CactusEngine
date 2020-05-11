#pragma once
#include "GBufferRenderNode.h"
#include "DrawingSystem.h"
#include "BaseRenderer.h"
#include "AllComponents.h"

using namespace Engine;

const char* GBufferRenderNode::OUTPUT_NORMAL_GBUFFER = "NormalGBufferTexture";
const char* GBufferRenderNode::OUTPUT_POSITION_GBUFFER = "PositionGBufferTexture";

GBufferRenderNode::GBufferRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
	: RenderNode(pGraphResources, pRenderer)
{

}

void GBufferRenderNode::SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources)
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// GBuffer color textures

	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.generateMipmap = false;
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler();
	texCreateInfo.textureWidth = screenWidth;
	texCreateInfo.textureHeight = screenHeight;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pPositionOutput);

	pGraphResources->Add(OUTPUT_NORMAL_GBUFFER, m_pNormalOutput);
	pGraphResources->Add(OUTPUT_POSITION_GBUFFER, m_pPositionOutput);

	// Depth attachment

	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;
	texCreateInfo.initialLayout = EImageLayout::DepthStencilAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pDepthBuffer);

	// Render pass object

	RenderPassAttachmentDescription normalDesc = {};
	normalDesc.format = ETextureFormat::RGBA32F;
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

	RenderPassAttachmentDescription positionDesc = {};
	positionDesc.format = ETextureFormat::RGBA32F;
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

	RenderPassAttachmentDescription depthDesc = {};
	depthDesc.format = ETextureFormat::Depth;
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

	RenderPassCreateInfo passCreateInfo = {};
	passCreateInfo.clearColor = Color4(1);
	passCreateInfo.clearDepth = 1.0f;
	passCreateInfo.clearStencil = 0;
	passCreateInfo.attachmentDescriptions.emplace_back(normalDesc);
	passCreateInfo.attachmentDescriptions.emplace_back(positionDesc);
	passCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

	m_pDevice->CreateRenderPassObject(passCreateInfo, m_pRenderPassObject);

	// Frame buffer

	FrameBufferCreateInfo fbCreateInfo = {};
	fbCreateInfo.attachments.emplace_back(m_pNormalOutput);
	fbCreateInfo.attachments.emplace_back(m_pPositionOutput);
	fbCreateInfo.attachments.emplace_back(m_pDepthBuffer);
	fbCreateInfo.framebufferWidth = screenWidth;
	fbCreateInfo.framebufferHeight = screenHeight;
	fbCreateInfo.pRenderPass = m_pRenderPassObject;

	m_pDevice->CreateFrameBuffer(fbCreateInfo, m_pFrameBuffer);

	// Uniform buffer

	uint32_t perSubmeshAllocation = m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan ? 4096 : 1;

	UniformBufferCreateInfo ubCreateInfo = {};
	ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * perSubmeshAllocation;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

	// Pipeline object

	// Vertex input state

	VertexInputBindingDescription vertexInputBindingDesc = {};
	vertexInputBindingDesc.binding = 0;
	vertexInputBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	vertexInputBindingDesc.inputRate = EVertexInputRate::PerVertex;

	VertexInputAttributeDescription positionAttributeDesc = {};
	positionAttributeDesc.binding = vertexInputBindingDesc.binding;
	positionAttributeDesc.location = DrawingDevice::ATTRIB_POSITION_LOCATION;
	positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
	positionAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription normalAttributeDesc = {};
	normalAttributeDesc.binding = vertexInputBindingDesc.binding;
	normalAttributeDesc.location = DrawingDevice::ATTRIB_NORMAL_LOCATION;
	normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
	normalAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription texcoordAttributeDesc = {};
	texcoordAttributeDesc.binding = vertexInputBindingDesc.binding;
	texcoordAttributeDesc.location = DrawingDevice::ATTRIB_TEXCOORD_LOCATION;
	texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
	texcoordAttributeDesc.format = ETextureFormat::RG32F;

	VertexInputAttributeDescription tangentAttributeDesc = {};
	tangentAttributeDesc.binding = vertexInputBindingDesc.binding;
	tangentAttributeDesc.location = DrawingDevice::ATTRIB_TANGENT_LOCATION;
	tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
	tangentAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription bitangentAttributeDesc = {};
	bitangentAttributeDesc.binding = vertexInputBindingDesc.binding;
	bitangentAttributeDesc.location = DrawingDevice::ATTRIB_BITANGENT_LOCATION;
	bitangentAttributeDesc.offset = VertexBufferCreateInfo::bitangentOffset;
	bitangentAttributeDesc.format = ETextureFormat::RGB32F;

	PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.bindingDescs = { vertexInputBindingDesc };
	vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc, bitangentAttributeDesc };

	std::shared_ptr<PipelineVertexInputState> pVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, pVertexInputState);

	// Input assembly state

	PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;
	inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

	std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState = nullptr;
	m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState);

	// Rasterization state

	PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
	rasterizationStateCreateInfo.enableDepthClamp = false;
	rasterizationStateCreateInfo.discardRasterizerResults = false;
	rasterizationStateCreateInfo.cullMode = ECullMode::Back;
	rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

	std::shared_ptr<PipelineRasterizationState> pRasterizationState = nullptr;
	m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pRasterizationState);

	// Depth stencil state

	PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.enableDepthTest = true;
	depthStencilStateCreateInfo.enableDepthWrite = true;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
	depthStencilStateCreateInfo.enableStencilTest = false;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState);

	// Multisample state

	PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.enableSampleShading = false;
	multisampleStateCreateInfo.sampleCount = 1;

	std::shared_ptr<PipelineMultisampleState> pMultisampleState = nullptr;
	m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, pMultisampleState);

	// Color blend state

	AttachmentColorBlendStateDescription attachmentNoBlendDesc = {};
	attachmentNoBlendDesc.enableBlend = false;

	PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);
	colorBlendStateCreateInfo.blendStateDescs.emplace_back(attachmentNoBlendDesc);

	std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
	m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

	// Viewport state

	PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.width = screenWidth;
	viewportStateCreateInfo.height = screenHeight;

	std::shared_ptr<PipelineViewportState> pViewportState;
	m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

	// Pipeline creation

	GraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.pShaderProgram = m_pRenderer->GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GBuffer);
	pipelineCreateInfo.pVertexInputState = pVertexInputState;
	pipelineCreateInfo.pInputAssemblyState = pInputAssemblyState;
	pipelineCreateInfo.pColorBlendState = pColorBlendState;
	pipelineCreateInfo.pRasterizationState = pRasterizationState;
	pipelineCreateInfo.pDepthStencilState = pDepthStencilState;
	pipelineCreateInfo.pMultisampleState = pMultisampleState;
	pipelineCreateInfo.pViewportState = pViewportState;
	pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

	std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
	m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);

	m_graphicsPipelines.emplace(EBuiltInShaderProgramType::GBuffer, pPipeline);
}

void GBufferRenderNode::RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext)
{
	m_pTransformMatrices_UB->ResetSubBufferAllocation();

	auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Transform));
	auto pCameraComp = std::static_pointer_cast<CameraComponent>(pRenderContext->pCamera->GetComponent(EComponentType::Camera));
	if (!pCameraComp || !pCameraTransform)
	{
		return;
	}
	Vector3 cameraPos = pCameraTransform->GetPosition();
	Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
	Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
		gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
		pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

	std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = m_pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

	m_pDevice->BeginRenderPass(m_pRenderPassObject, m_pFrameBuffer, pCommandBuffer);

	m_pDevice->BindGraphicsPipeline(m_graphicsPipelines.at(EBuiltInShaderProgramType::GBuffer), pCommandBuffer);

	// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
	auto pShaderProgram = (m_pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GBuffer);

	UBTransformMatrices ubTransformMatrices = {};
	ubTransformMatrices.projectionMatrix = projectionMat;
	ubTransformMatrices.viewMatrix = viewMat;

	for (auto& entity : *pRenderContext->pDrawList)
	{
		auto pMaterialComp   = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
		auto pTransformComp  = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
		auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

		if (!pTransformComp || !pMeshFilterComp)
		{
			continue;
		}

		auto pMesh = pMeshFilterComp->GetMesh();

		if (!pMesh)
		{
			continue;
		}

		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
		ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();

		if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
		{
			auto pSubTransformMatricesUB = m_pTransformMatrices_UB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
		}
		else
		{
			m_pTransformMatrices_UB->UpdateBufferData(&ubTransformMatrices);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, m_pTransformMatrices_UB);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
		m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

		auto subMeshes = pMesh->GetSubMeshes();
		for (size_t i = 0; i < subMeshes->size(); ++i)
		{
			auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

			if (pMaterial->IsTransparent())
			{
				continue;
			}

			m_pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
		}

		if (m_eGraphicsDeviceType != EGraphicsDeviceType::Vulkan)
		{
			pShaderProgram->Reset();
		}
	}

	if (m_eGraphicsDeviceType == EGraphicsDeviceType::Vulkan)
	{
		m_pDevice->EndRenderPass(pCommandBuffer);
		m_pDevice->EndCommandBuffer(pCommandBuffer);

		m_pRenderer->WriteCommandRecordList(m_pName, pCommandBuffer);
	}
}