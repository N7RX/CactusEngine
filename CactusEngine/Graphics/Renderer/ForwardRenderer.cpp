#include "ForwardRenderer.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "CameraComponent.h"
#include "DrawingSystem.h"
#include "Timer.h"

using namespace Engine;

ForwardRenderer::ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(eRenderer_Forward, pDevice, pSystem)
{
}

void ForwardRenderer::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>();

	BuildFrameResources();

	BuildOpaquePass();
	BuildTransparentPass();
	BuildOpaqueTranspBlendPass();
}

void ForwardRenderer::BuildFrameResources()
{
	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.textureWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowWidth();
	texCreateInfo.textureHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowHeight();

	// Color output from opaque pass

	texCreateInfo.dataType = eDataType_Float;
	texCreateInfo.format = eFormat_RGBA32F;
	texCreateInfo.textureType = eTextureType_ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassColorOutput);

	// Depth output from opaque pass

	texCreateInfo.dataType = eDataType_Float;
	texCreateInfo.format = eFormat_Depth;
	texCreateInfo.textureType = eTextureType_DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassDepthOutput);

	// Frame buffer for opaque pass

	FrameBufferCreateInfo opaqueFBCreateInfo = {};
	opaqueFBCreateInfo.bindTextures.emplace_back(m_pOpaquePassColorOutput);
	opaqueFBCreateInfo.bindTextures.emplace_back(m_pOpaquePassDepthOutput);
	opaqueFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	opaqueFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(opaqueFBCreateInfo, m_pOpaquePassFrameBuffer);

	// Color output from transparent pass

	texCreateInfo.dataType = eDataType_Float;
	texCreateInfo.format = eFormat_RGBA32F;
	texCreateInfo.textureType = eTextureType_ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassColorOutput);

	// Depth output from transparent pass

	texCreateInfo.dataType = eDataType_Float;
	texCreateInfo.format = eFormat_Depth;
	texCreateInfo.textureType = eTextureType_DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassDepthOutput);

	// Frame buffer for transparent pass

	FrameBufferCreateInfo transpFBCreateInfo = {};
	transpFBCreateInfo.bindTextures.emplace_back(m_pTranspPassColorOutput);
	transpFBCreateInfo.bindTextures.emplace_back(m_pTranspPassDepthOutput);
	transpFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	transpFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(transpFBCreateInfo, m_pTranspPassFrameBuffer);
}

void ForwardRenderer::BuildOpaquePass()
{
	RenderGraphResource opaquePassInput;
	RenderGraphResource opaquePassOutput;

	opaquePassInput.Add(RGraphResName::FWD_OPAQUE_FB, m_pOpaquePassFrameBuffer);

	opaquePassOutput.Add(RGraphResName::FWD_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	opaquePassOutput.Add(RGraphResName::FWD_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);	

	auto pOpaquePass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
	{
		auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(eCompType_Transform));
		auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(eCompType_Camera));
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(RGraphResName::FWD_OPAQUE_FB)));

		for (auto& entity : *pContext->pDrawList)
		{
			auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(eCompType_Material));

			if (!pMaterialComp || pMaterialComp->IsTransparent())
			{
				continue;
			}

			auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(eCompType_Transform));
			auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(eCompType_MeshFilter));
			
			if (!pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			auto pMesh = pMeshFilterComp->GetMesh();

			if (!pMesh)
			{
				continue;
			}

			Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
			Matrix3x3 normalMat = pTransformComp->GetNormalMatrix();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterialComp->GetShaderProgramType());
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			float currTime = Timer::Now();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), eShaderParam_Mat4, glm::value_ptr(modelMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), eShaderParam_Mat4, glm::value_ptr(viewMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PROJECTION_MATRIX), eShaderParam_Mat4, glm::value_ptr(projectionMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NORMAL_MATRIX), eShaderParam_Mat3, glm::value_ptr(normalMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_POSITION), eShaderParam_Vec3, glm::value_ptr(cameraPos));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TIME), eShaderParam_Float1, &currTime);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), eShaderParam_Vec4, glm::value_ptr(pMaterialComp->GetAlbedoColor()));

			auto pAlbedoTexture = pMaterialComp->GetTexture(eMaterialTexture_Albedo);
			if (pAlbedoTexture)
			{
				// Alert: this only works for OpenGL; Vulkan requires doing this through descriptor sets
				uint32_t texID = pAlbedoTexture->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), eShaderParam_Texture2D, &texID);
			}

			auto pNoiseTexture = pMaterialComp->GetTexture(eMaterialTexture_Noise);
			if (pNoiseTexture)
			{
				uint32_t texID = pNoiseTexture->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE), eShaderParam_Texture2D, &texID);
			}

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());
			pDevice->DrawPrimitive(pMesh->GetVertexBuffer()->GetNumberOfIndices());

			pShaderProgram->Reset();
		}
	},
		opaquePassInput,
		opaquePassOutput);

	m_pRenderGraph->AddRenderNode(eRenderNode_Opaque, pOpaquePass);
}

void ForwardRenderer::BuildTransparentPass()
{
	RenderGraphResource transparentPassInput;
	RenderGraphResource transparentPassOutput;

	transparentPassInput.Add(RGraphResName::FWD_TRANSP_FB, m_pTranspPassFrameBuffer);
	transparentPassInput.Add(RGraphResName::FWD_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	transparentPassInput.Add(RGraphResName::FWD_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	transparentPassOutput.Add(RGraphResName::FWD_TRANSP_COLOR, m_pTranspPassColorOutput);
	transparentPassOutput.Add(RGraphResName::FWD_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pTransparentPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
	{
		auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(eCompType_Transform));
		auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(eCompType_Camera));
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = true;
		blendInfo.srcFactor = eBlend_SrcAlpha;
		blendInfo.dstFactor = eBlend_OneMinusSrcAlpha;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(RGraphResName::FWD_TRANSP_FB)));

		for (auto& entity : *pContext->pDrawList)
		{
			auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(eCompType_Material));

			if (!pMaterialComp || !pMaterialComp->IsTransparent())
			{
				continue;
			}

			auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(eCompType_Transform));
			auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(eCompType_MeshFilter));

			if (!pTransformComp || !pMeshFilterComp)
			{
				continue;
			}

			auto pMesh = pMeshFilterComp->GetMesh();

			if (!pMesh)
			{
				continue;
			}

			Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
			Matrix3x3 normalMat = pTransformComp->GetNormalMatrix();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterialComp->GetShaderProgramType());
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			float currTime = Timer::Now();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), eShaderParam_Mat4, glm::value_ptr(modelMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), eShaderParam_Mat4, glm::value_ptr(viewMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PROJECTION_MATRIX), eShaderParam_Mat4, glm::value_ptr(projectionMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NORMAL_MATRIX), eShaderParam_Mat3, glm::value_ptr(normalMat));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_POSITION), eShaderParam_Vec3, glm::value_ptr(cameraPos));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TIME), eShaderParam_Float1, &currTime);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), eShaderParam_Vec4, glm::value_ptr(pMaterialComp->GetAlbedoColor()));

			auto depthTextureID = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_OPAQUE_DEPTH))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_1), eShaderParam_Texture2D, &depthTextureID);

			auto colorTextureID = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_OPAQUE_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), eShaderParam_Texture2D, &colorTextureID);

			auto pAlbedoTexture = pMaterialComp->GetTexture(eMaterialTexture_Albedo);
			if (pAlbedoTexture)
			{
				// Alert: this only works for OpenGL; Vulkan requires doing this through descriptor sets
				uint32_t texID = pAlbedoTexture->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), eShaderParam_Texture2D, &texID);
			}

			auto pNoiseTexture = pMaterialComp->GetTexture(eMaterialTexture_Noise);
			if (pNoiseTexture)
			{
				uint32_t texID = pNoiseTexture->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE), eShaderParam_Texture2D, &texID);
			}

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());
			pDevice->DrawPrimitive(pMesh->GetVertexBuffer()->GetNumberOfIndices());

			pShaderProgram->Reset();
		}
	},
		transparentPassInput,
		transparentPassOutput);

	auto pOpaquePass = m_pRenderGraph->GetNodeByType(eRenderNode_Opaque);
	pOpaquePass->AddNextNode(pTransparentPass);

	m_pRenderGraph->AddRenderNode(eRenderNode_Transparent, pTransparentPass);
}

void ForwardRenderer::BuildOpaqueTranspBlendPass()
{
	RenderGraphResource blendPassInput;
	RenderGraphResource blendPassOutput;

	blendPassInput.Add(RGraphResName::FWD_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	blendPassInput.Add(RGraphResName::FWD_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	blendPassInput.Add(RGraphResName::FWD_TRANSP_COLOR, m_pTranspPassColorOutput);
	blendPassInput.Add(RGraphResName::FWD_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pBlendPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
	{
		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(nullptr); // Alert: this won't work for Vulkan

		auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(eShaderProgram_DepthBased_ColorBlend_2);
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		auto depthTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_OPAQUE_DEPTH))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_1), eShaderParam_Texture2D, &depthTextureID_1);

		auto colorTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_OPAQUE_COLOR))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), eShaderParam_Texture2D, &colorTextureID_1);

		auto depthTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_TRANSP_DEPTH))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_2), eShaderParam_Texture2D, &depthTextureID_2);

		auto colorTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(RGraphResName::FWD_TRANSP_COLOR))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_2), eShaderParam_Texture2D, &colorTextureID_2);

		pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
		pDevice->DrawFullScreenQuad();

		pShaderProgram->Reset();
	},
		blendPassInput,
		blendPassOutput);

	auto pTransparentPass = m_pRenderGraph->GetNodeByType(eRenderNode_Transparent);
	pTransparentPass->AddNextNode(pBlendPass);

	m_pRenderGraph->AddRenderNode(eRenderNode_ColorBlend_DepthBased_2, pBlendPass);
}

void ForwardRenderer::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pContext = std::make_shared<RenderContext>();
	pContext->pCamera = pCamera;
	pContext->pDrawList = &drawList;
	pContext->pRenderer = this;

	m_pRenderGraph->BeginRenderPasses(pContext);
}