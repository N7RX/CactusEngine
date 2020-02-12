#include "ForwardRenderer.h"
#include "DrawingSystem.h"
#include "AllComponents.h"
#include "Timer.h"
#include "ImageTexture.h"

// For line drawing computation
#include <Eigen/Dense>

using namespace Engine;

ForwardRenderer::ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(ERendererType::Forward, pDevice, pSystem)
{
}

void ForwardRenderer::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>();

	BuildFrameResources();

	BuildShadowMapPass();
	BuildNormalOnlyPass();
	BuildOpaquePass();
	BuildGaussianBlurPass();
	BuildLineDrawingPass();
	BuildTransparentPass();
	BuildOpaqueTranspBlendPass();
	BuildDepthOfFieldPass();
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

// TODO: rewrite this function, this is way too long and high-coupling
void ForwardRenderer::BuildFrameResources()
{
	auto screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	auto screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	Texture2DCreateInfo texCreateInfo = {};

	// Depth attachment for shadow map pass

	texCreateInfo.textureWidth = 4096; // Shadow map resolution
	texCreateInfo.textureHeight = 4096;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pShadowMapPassDepthOutput);

	// Frame buffer for shadow map pass

	FrameBufferCreateInfo shadowMapFBCreateInfo = {};
	shadowMapFBCreateInfo.attachments.emplace_back(m_pShadowMapPassDepthOutput);
	shadowMapFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	shadowMapFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(shadowMapFBCreateInfo, m_pShadowMapPassFrameBuffer);

	texCreateInfo.textureWidth = screenWidth;
	texCreateInfo.textureHeight = screenHeight;

	// Color outputs from normal-only pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassNormalOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassPositionOutput);

	// Depth attachment for normal-only pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassDepthOutput);

	// Frame buffer for normal-only pass

	FrameBufferCreateInfo normalOnlyFBCreateInfo = {};
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassNormalOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassPositionOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassDepthOutput);
	normalOnlyFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	normalOnlyFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(normalOnlyFBCreateInfo, m_pNormalOnlyPassFrameBuffer);

	// Color output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassColorOutput);

	// Shadow color output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassShadowOutput);

	// Depth output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassDepthOutput);

	// Frame buffer for opaque pass

	FrameBufferCreateInfo opaqueFBCreateInfo = {};
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassColorOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassShadowOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassDepthOutput);
	opaqueFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	opaqueFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(opaqueFBCreateInfo, m_pOpaquePassFrameBuffer);

	// Color outputs from Gaussian blur pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassHorizontalColorOutput);

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassFinalColorOutput);

	// Frame buffer for Gaussian blur pass

	FrameBufferCreateInfo blurFBCreateInfo = {};
	blurFBCreateInfo.attachments.emplace_back(m_pBlurPassHorizontalColorOutput);
	blurFBCreateInfo.attachments.emplace_back(m_pBlurPassFinalColorOutput);
	blurFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	blurFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(blurFBCreateInfo, m_pBlurPassFrameBuffer);

	// Curvature output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassCurvatureOutput);

	// Blurred output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassBlurredOutput);

	// Color output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassColorOutput);

	// Frame buffer for line drawing pass

	FrameBufferCreateInfo lineDrawFBCreateInfo = {};
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassBlurredOutput);
	lineDrawFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	lineDrawFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo, m_pLineDrawingPassFrameBuffer);

	// Color output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassColorOutput);

	// Depth output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassDepthOutput);

	// Frame buffer for transparent pass

	FrameBufferCreateInfo transpFBCreateInfo = {};
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassColorOutput);
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassDepthOutput);
	transpFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	transpFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(transpFBCreateInfo, m_pTranspPassFrameBuffer);

	// Color output from blend pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlendPassColorOutput);

	// Frame buffer for blend pass

	FrameBufferCreateInfo blendFBCreateInfo = {};
	blendFBCreateInfo.attachments.emplace_back(m_pBlendPassColorOutput);
	blendFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	blendFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(blendFBCreateInfo, m_pBlendPassFrameBuffer);

	// Horizontal color output from DOF pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pDOFPassHorizontalOutput);

	// Frame buffer for DOF pass

	FrameBufferCreateInfo dofFBCreateInfo = {};
	dofFBCreateInfo.attachments.emplace_back(m_pDOFPassHorizontalOutput);
	dofFBCreateInfo.framebufferWidth = texCreateInfo.textureWidth;
	dofFBCreateInfo.framebufferHeight = texCreateInfo.textureHeight;

	m_pDevice->CreateFrameBuffer(dofFBCreateInfo, m_pDOFPassFrameBuffer);

	// Post effects resources
	m_pBrushMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_1.png");
	m_pBrushMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_2.png");
	m_pPencilMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_1.jpg");
	m_pPencilMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_2.jpg");
}

void ForwardRenderer::BuildShadowMapPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::SHADOWMAP_FB, m_pShadowMapPassFrameBuffer);

	passOutput.Add(ForwardGraphRes::SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);

	auto pShadowMapPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			Vector3 lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
			auto screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Change viewport to shadow map size
			pDevice->ResizeViewPort(4096, 4096);

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::SHADOWMAP_FB)));
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
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

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				auto subMeshes = pMesh->GetSubMeshes();
				for (unsigned int i = 0; i < subMeshes->size(); ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), EShaderParamType::Mat4, glm::value_ptr(modelMat));
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::LIGHT_SPACE_MATRIX), EShaderParamType::Mat4, glm::value_ptr(lightSpaceMatrix));

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						uint32_t texID = pAlbedoTexture->GetTextureID();
						pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), EShaderParamType::Texture2D, &texID);
					}

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}

				pShaderProgram->Reset();
			}

			pDevice->ResizeViewPort(screenWidth, screenHeight);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::SHADOWMAP_NODE, pShadowMapPass);
}

void ForwardRenderer::BuildNormalOnlyPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::NORMALONLY_FB, m_pNormalOnlyPassFrameBuffer);

	passOutput.Add(ForwardGraphRes::NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passOutput.Add(ForwardGraphRes::NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);

	auto pNormalOnlyPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::NORMALONLY_FB)));
			pDevice->ClearRenderTarget();

			// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
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

				Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
				Matrix3x3 normalMat = pTransformComp->GetNormalMatrix();

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), EShaderParamType::Mat4, glm::value_ptr(modelMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), EShaderParamType::Mat4, glm::value_ptr(viewMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PROJECTION_MATRIX), EShaderParamType::Mat4, glm::value_ptr(projectionMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NORMAL_MATRIX), EShaderParamType::Mat3, glm::value_ptr(normalMat));

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				auto subMeshes = pMesh->GetSubMeshes();
				for (size_t i = 0; i < subMeshes->size(); ++i)
				{
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);
				}

				pShaderProgram->Reset();
			}
		},
		passInput,
		passOutput);

	auto pShadowMapPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::SHADOWMAP_NODE);
	pShadowMapPass->AddNextNode(pNormalOnlyPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NORMALONLY_NODE, pNormalOnlyPass);
}

void ForwardRenderer::BuildOpaquePass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::OPAQUE_FB, m_pOpaquePassFrameBuffer);
	passInput.Add(ForwardGraphRes::NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passInput.Add(ForwardGraphRes::SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);

	passOutput.Add(ForwardGraphRes::OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passOutput.Add(ForwardGraphRes::OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passOutput.Add(ForwardGraphRes::OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	auto pOpaquePass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			Vector3 lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto shadowMapID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::SHADOWMAP_DEPTH))->GetTextureID();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::OPAQUE_FB)));
			pDevice->ClearRenderTarget();

			float currTime = Timer::Now();

			for (auto& entity : *pContext->pDrawList)
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
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

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				unsigned int submeshCount = pMesh->GetSubmeshCount();
				auto subMeshes = pMesh->GetSubMeshes();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				for (unsigned int i = 0; i < submeshCount; ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
				pShaderParamTable->Clear();

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), EShaderParamType::Mat4, glm::value_ptr(modelMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), EShaderParamType::Mat4, glm::value_ptr(viewMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PROJECTION_MATRIX), EShaderParamType::Mat4, glm::value_ptr(projectionMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NORMAL_MATRIX), EShaderParamType::Mat3, glm::value_ptr(normalMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_POSITION), EShaderParamType::Vec3, glm::value_ptr(cameraPos));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TIME), EShaderParamType::Float1, &currTime);

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EShaderParamType::Texture2D, &shadowMapID);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::LIGHT_SPACE_MATRIX), EShaderParamType::Mat4, glm::value_ptr(lightSpaceMatrix));

				auto anisotropy = pMaterial->GetAnisotropy();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ANISOTROPY), EShaderParamType::Float1, &anisotropy);

				auto roughness = pMaterial->GetRoughness();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ROUGHNESS), EShaderParamType::Float1, &roughness);

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), EShaderParamType::Vec4, glm::value_ptr(pMaterial->GetAlbedoColor()));

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					uint32_t texID = pAlbedoTexture->GetTextureID();
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), EShaderParamType::Texture2D, &texID);
				}

				auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
				if (pNoiseTexture)
				{
					uint32_t texID = pNoiseTexture->GetTextureID();
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE_1), EShaderParamType::Texture2D, &texID);
				}

				auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
				if (pToneTexture)
				{
					uint32_t texID = pToneTexture->GetTextureID();
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TONE_TEXTURE), EShaderParamType::Texture2D, &texID);
				}

				auto gNormalTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::NORMALONLY_NORMAL))->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::GNORMAL_TEXTURE), EShaderParamType::Texture2D, &gNormalTextureID);

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
				pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);

				pShaderProgram->Reset();
			}
			
		}
	},
		passInput,
		passOutput);

	auto pNormalOnlyPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NORMALONLY_NODE);
	pNormalOnlyPass->AddNextNode(pOpaquePass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::OPAQUE_NODE, pOpaquePass);
}

void ForwardRenderer::BuildGaussianBlurPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::BLUR_FB, m_pBlurPassFrameBuffer);
	passInput.Add(ForwardGraphRes::OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(ForwardGraphRes::BLUR_HORIZONTAL_COLOR, m_pBlurPassHorizontalColorOutput);

	passOutput.Add(ForwardGraphRes::BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);

	auto pGaussianBlurPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Horizontal subpass

			// Set render target to color attachment 0
			std::vector<uint32_t> attachmentIndex = { 0 };
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::BLUR_FB)), attachmentIndex); // Alert: this won't work for Vulkan
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			auto colorTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::OPAQUE_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID_1);

			int horizontal = 1;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Vertical subpass

			// Set render target to color attachment 1
			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::BLUR_FB)), attachmentIndex);

			pShaderParamTable->Clear();

			auto colorTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::BLUR_HORIZONTAL_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID_2);

			horizontal = 0;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();
		},
		passInput,
		passOutput);

	auto pOpaquePass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::OPAQUE_NODE);
	pOpaquePass->AddNextNode(pGaussianBlurPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::BLUR_NODE, pGaussianBlurPass);
}

void ForwardRenderer::BuildLineDrawingPass()
{
	CreateLineDrawingMatrices(); // Pre-calculate local parameterization matrices

	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::LINEDRAWING_FB, m_pLineDrawingPassFrameBuffer);
	passInput.Add(ForwardGraphRes::BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);
	passInput.Add(ForwardGraphRes::OPAQUE_COLOR, m_pOpaquePassColorOutput);	
	passInput.Add(ForwardGraphRes::LINEDRAWING_MATRIX_TEXTURE, m_pLineDrawingMatrixTexture);
	passInput.Add(ForwardGraphRes::LINEDRAWING_CURVATURE, m_pLineDrawingPassCurvatureOutput);
	passInput.Add(ForwardGraphRes::LINEDRAWING_BLURRED, m_pLineDrawingPassBlurredOutput);
	passInput.Add(ForwardGraphRes::LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);

	passOutput.Add(ForwardGraphRes::LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);

	auto pLineDrawingPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Curvature computation subpass

			// Set render target
			std::vector<uint32_t> attachmentIndex = { 0 };
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::LINEDRAWING_FB)), attachmentIndex); // Alert: this won't work for Vulkan
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			auto colorTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::BLUR_FINAL_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID_1);

			auto matrixTexID = std::static_pointer_cast<RenderTexture>(input.Get(ForwardGraphRes::LINEDRAWING_MATRIX_TEXTURE))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::SAMPLE_MATRIX_TEXTURE), EShaderParamType::Texture2D, &matrixTexID);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Ridge searching subpass

			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::LINEDRAWING_FB)), attachmentIndex);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
			pShaderParamTable->Clear();

			auto curvatureTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::LINEDRAWING_CURVATURE))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &curvatureTextureID);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Gaussian blur subpasses

			// Horizontal subpass

			// Set render target to curvature color attachment
			attachmentIndex[0] = 0;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::LINEDRAWING_FB)), attachmentIndex);
			pDevice->ClearRenderTarget();

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			pShaderParamTable->Clear();

			auto colorTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::LINEDRAWING_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID);

			int horizontal = 1;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Vertical subpass

			// Set render target to blurred color attachment
			attachmentIndex[0] = 2;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::LINEDRAWING_FB)), attachmentIndex);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &curvatureTextureID);

			horizontal = 0;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Final blend subpass

			// Set render target to color attachment
			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::LINEDRAWING_FB)), attachmentIndex);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
			pShaderParamTable->Clear();

			auto blurredTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::LINEDRAWING_BLURRED))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &blurredTextureID);

			auto colorTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::OPAQUE_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_2), EShaderParamType::Texture2D, &colorTextureID_2);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

		},
		passInput,
		passOutput);

	auto pBlurPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::BLUR_NODE);
	pBlurPass->AddNextNode(pLineDrawingPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::LINEDRAWING_NODE, pLineDrawingPass);
}

void ForwardRenderer::BuildTransparentPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::TRANSP_FB, m_pTranspPassFrameBuffer);
	passInput.Add(ForwardGraphRes::LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(ForwardGraphRes::OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	passOutput.Add(ForwardGraphRes::TRANSP_COLOR, m_pTranspPassColorOutput);
	passOutput.Add(ForwardGraphRes::TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pTransparentPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
	{
		auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
		auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
		if (!pCameraComp || !pCameraTransform)
		{
			return;
		}
		Vector3 cameraPos = pCameraTransform->GetPosition();
		Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
		Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
			pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

		float currTime = Timer::Now();

		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = true;
		blendInfo.srcFactor = EBlendFactor::SrcAlpha;
		blendInfo.dstFactor = EBlendFactor::OneMinusSrcAlpha;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::TRANSP_FB)));
		pDevice->ClearRenderTarget();

		for (auto& entity : *pContext->pDrawList)
		{
			auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
			auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
			auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

			if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
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
	
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			unsigned int submeshCount = pMesh->GetSubmeshCount();
			auto subMeshes = pMesh->GetSubMeshes();

			pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

			for (unsigned int i = 0; i < submeshCount; ++i)
			{
				auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

				if (!pMaterial->IsTransparent()) // TODO: use a list to record these parts instead of checking one-by-one
				{
					continue;
				}

				auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
				pShaderParamTable->Clear();

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MODEL_MATRIX), EShaderParamType::Mat4, glm::value_ptr(modelMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), EShaderParamType::Mat4, glm::value_ptr(viewMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PROJECTION_MATRIX), EShaderParamType::Mat4, glm::value_ptr(projectionMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NORMAL_MATRIX), EShaderParamType::Mat3, glm::value_ptr(normalMat));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_POSITION), EShaderParamType::Vec3, glm::value_ptr(cameraPos));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TIME), EShaderParamType::Float1, &currTime);

				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), EShaderParamType::Vec4, glm::value_ptr(pMaterial->GetAlbedoColor()));

				auto depthTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::OPAQUE_DEPTH))->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_1), EShaderParamType::Texture2D, &depthTextureID);

				auto colorTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::LINEDRAWING_COLOR))->GetTextureID();
				pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID);

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					// Alert: this only works for OpenGL; Vulkan requires doing this through descriptor sets
					uint32_t texID = pAlbedoTexture->GetTextureID();
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), EShaderParamType::Texture2D, &texID);
				}

				auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
				if (pNoiseTexture)
				{
					uint32_t texID = pNoiseTexture->GetTextureID();
					pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE_1), EShaderParamType::Texture2D, &texID);
				}

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
				pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);

				pShaderProgram->Reset();
			}
			
		}
	},
		passInput,
		passOutput);

	auto pLineDrawingPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::LINEDRAWING_NODE);
	pLineDrawingPass->AddNextNode(pTransparentPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::TRANSP_NODE, pTransparentPass);
}

void ForwardRenderer::BuildOpaqueTranspBlendPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::BLEND_FB, m_pBlendPassFrameBuffer);
	passInput.Add(ForwardGraphRes::LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(ForwardGraphRes::OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(ForwardGraphRes::TRANSP_COLOR, m_pTranspPassColorOutput);
	passInput.Add(ForwardGraphRes::TRANSP_DEPTH, m_pTranspPassDepthOutput);

	passOutput.Add(ForwardGraphRes::BLEND_COLOR, m_pBlendPassColorOutput);

	auto pBlendPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
	{
		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::BLEND_FB)));
		pDevice->ClearRenderTarget();

		auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		auto depthTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::OPAQUE_DEPTH))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_1), EShaderParamType::Texture2D, &depthTextureID_1);
		
		auto colorTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::LINEDRAWING_COLOR))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID_1);

		auto depthTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TRANSP_DEPTH))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::DEPTH_TEXTURE_2), EShaderParamType::Texture2D, &depthTextureID_2);

		auto colorTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TRANSP_COLOR))->GetTextureID();
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_2), EShaderParamType::Texture2D, &colorTextureID_2);

		pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
		pDevice->DrawFullScreenQuad();

		pShaderProgram->Reset();
	},
		passInput,
		passOutput);

	auto pTransparentPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::TRANSP_NODE);
	pTransparentPass->AddNextNode(pBlendPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::COLORBLEND_D2_NODE, pBlendPass);
}

void ForwardRenderer::BuildDepthOfFieldPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::DOF_FB, m_pDOFPassFrameBuffer);
	passInput.Add(ForwardGraphRes::BLEND_COLOR, m_pBlendPassColorOutput);
	passInput.Add(ForwardGraphRes::NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);
	passInput.Add(ForwardGraphRes::DOF_HORIZONTAL, m_pDOFPassHorizontalOutput);
	passInput.Add(ForwardGraphRes::BRUSH_MASK_TEXTURE_1, m_pBrushMaskImageTexture_1);
	passInput.Add(ForwardGraphRes::BRUSH_MASK_TEXTURE_2, m_pBrushMaskImageTexture_2);
	passInput.Add(ForwardGraphRes::PENCIL_MASK_TEXTURE_1, m_pPencilMaskImageTexture_1);
	passInput.Add(ForwardGraphRes::PENCIL_MASK_TEXTURE_2, m_pPencilMaskImageTexture_2);
	passInput.Add(ForwardGraphRes::OPAQUE_SHADOW, m_pOpaquePassShadowOutput);

	auto pDOFPass = std::make_shared<RenderNode>(m_pRenderGraph,
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			float currTime = Timer::Now();

			// Horizontal subpass

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::DOF_FB)));
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), EShaderParamType::Mat4, glm::value_ptr(viewMat));

			float aperture = pCameraComp->GetAperture();
			float focalDistance = pCameraComp->GetFocalDistance();
			float imageDistance = pCameraComp->GetImageDistance();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_APERTURE), EShaderParamType::Float1, &aperture);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_FOCALDISTANCE), EShaderParamType::Float1, &focalDistance);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::CAMERA_IMAGEDISTANCE), EShaderParamType::Float1, &imageDistance);

			auto colorTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::BLEND_COLOR))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &colorTextureID_1);

			auto gPositionTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::NORMALONLY_POSITION))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::GPOSITION_TEXTURE), EShaderParamType::Texture2D, &gPositionTextureID);

			int horizontal = 1;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			// Vertical subpass

			// Set to direct output
			pDevice->SetRenderTarget(nullptr); // Alert: this won't work for Vulkan

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::VIEW_MATRIX), EShaderParamType::Mat4, glm::value_ptr(viewMat));

			auto horizontalResultTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::DOF_HORIZONTAL))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_1), EShaderParamType::Texture2D, &horizontalResultTextureID);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::GPOSITION_TEXTURE), EShaderParamType::Texture2D, &gPositionTextureID);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::TIME), EShaderParamType::Float1, &currTime);

			auto brushMaskTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::BRUSH_MASK_TEXTURE_1))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MASK_TEXTURE_1), EShaderParamType::Texture2D, &brushMaskTextureID_1);

			auto brushMaskTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::BRUSH_MASK_TEXTURE_2))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE_1), EShaderParamType::Texture2D, &brushMaskTextureID_2);

			auto pencilMaskTextureID_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::PENCIL_MASK_TEXTURE_1))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::MASK_TEXTURE_2), EShaderParamType::Texture2D, &pencilMaskTextureID_1);

			auto pencilMaskTextureID_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::PENCIL_MASK_TEXTURE_2))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::NOISE_TEXTURE_2), EShaderParamType::Texture2D, &pencilMaskTextureID_2);

			auto shadowResultTextureID = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::OPAQUE_SHADOW))->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::COLOR_TEXTURE_2), EShaderParamType::Texture2D, &shadowResultTextureID);

			horizontal = 0;
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::BOOL_1), EShaderParamType::Int1, &horizontal);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();
		},
		passInput,
		passOutput);

	auto pBlendPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::COLORBLEND_D2_NODE);
	pBlendPass->AddNextNode(pDOFPass);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::DOF_NODE, pDOFPass);
}

void ForwardRenderer::CreateLineDrawingMatrices()
{
	// Alert: current implementation does not support window scaling

	auto width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	auto height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Storing the 6x9 sampling matrix
	// But a5 is not used, so we are only storing 5x9
	std::vector<float> linearResults;
	linearResults.resize(12 * 4, 0);
	m_pLineDrawingMatrixTexture = std::make_shared<RenderTexture>(12, 1); // Store the matrix in a 4-channel linear texture

	float xPixelSize = 1.0f / width;
	float yPixelSize = 1.0f / height;

	Eigen::MatrixXf H(6, 9);
	Eigen::MatrixXf X(9, 6);

	// 3x3 grid sample
	int rowIndex = 0;
	int lineWidth = 1;
	for (int m = -lineWidth; m < lineWidth + 1; m += lineWidth)
	{
		for (int n = -lineWidth; n < lineWidth + 1; n += lineWidth)
		{
			float xSample = m * xPixelSize;
			float ySample = n * yPixelSize;

			X(rowIndex, 0) = xSample * xSample;
			X(rowIndex, 1) = 2 * xSample * ySample;
			X(rowIndex, 2) = ySample * ySample;
			X(rowIndex, 3) = xSample;
			X(rowIndex, 4) = ySample;
			X(rowIndex, 5) = 1;

			rowIndex++;
		}
	}

	H = (X.transpose() * X).inverse() * X.transpose();

	// H is divided as
	// | 1 | 2 |   |
	// | 3 | 4 |   |
	// | 5 | 6 | 11|
	// | 7 | 8 |---|
	// | 9 | 10| 12|

	int linearIndex = 0;
	for (int p = 0; p < 5; ++p) // a5 is not used, so instead of 6 rows, 5 rows are taken
	{
		for (int q = 0; q < 8; ++q)
		{
			linearResults[linearIndex] = H(p, q);
			linearIndex++;
		}
	}

	// Store the 9th column
	for (int p = 0; p < 5; ++p)
	{
		linearResults[linearIndex] = H(p, 8);
		linearIndex++;
	}

	m_pLineDrawingMatrixTexture->FlushData(linearResults.data(), EDataType::Float32, ETextureFormat::RGBA32F);
}