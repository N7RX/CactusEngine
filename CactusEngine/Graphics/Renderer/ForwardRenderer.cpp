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
	m_pRenderGraph = std::make_shared<RenderGraph>(m_pDevice, 0);

	BuildFrameResources();

	BuildShadowMapPass();
	BuildNormalOnlyPass();
	BuildOpaquePass();
	BuildGaussianBlurPass();
	BuildLineDrawingPass();
	BuildTransparentPass();
	BuildOpaqueTranspBlendPass();
	BuildDepthOfFieldPass();

	BuildRenderNodeDependencies();
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

void ForwardRenderer::BuildFrameResources()
{
	CreateFrameTextures();
	CreateFrameBuffers();
	CreateUniformBuffers();
}

void ForwardRenderer::CreateFrameTextures()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	Texture2DCreateInfo texCreateInfo = {};

	// Depth attachment for shadow map pass

	texCreateInfo.textureWidth = SHADOW_MAP_RESOLUTION; // Shadow map resolution
	texCreateInfo.textureHeight = SHADOW_MAP_RESOLUTION;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pShadowMapPassDepthOutput);

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

	// Color output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassColorOutput);

	// Shadow color output from opaque pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassShadowOutput);

	// Depth output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassDepthOutput);

	// Color outputs from Gaussian blur pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassHorizontalColorOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassFinalColorOutput);

	// Curvature output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassCurvatureOutput);

	// Blurred output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassBlurredOutput);

	// Color output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassColorOutput);

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

	// Color output from blend pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlendPassColorOutput);

	// Horizontal color output from DOF pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pDOFPassHorizontalOutput);

	// Post effects resources
	m_pBrushMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_1.png");
	m_pBrushMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_2.png");
	m_pPencilMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_1.jpg");
	m_pPencilMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_2.jpg");
}

void ForwardRenderer::CreateFrameBuffers()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Frame buffer for shadow map pass

	FrameBufferCreateInfo shadowMapFBCreateInfo = {};
	shadowMapFBCreateInfo.attachments.emplace_back(m_pShadowMapPassDepthOutput);
	shadowMapFBCreateInfo.framebufferWidth = SHADOW_MAP_RESOLUTION;
	shadowMapFBCreateInfo.framebufferHeight = SHADOW_MAP_RESOLUTION;

	m_pDevice->CreateFrameBuffer(shadowMapFBCreateInfo, m_pShadowMapPassFrameBuffer);

	// Frame buffer for normal-only pass

	FrameBufferCreateInfo normalOnlyFBCreateInfo = {};
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassNormalOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassPositionOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassDepthOutput);
	normalOnlyFBCreateInfo.framebufferWidth = screenWidth;
	normalOnlyFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(normalOnlyFBCreateInfo, m_pNormalOnlyPassFrameBuffer);

	// Frame buffer for opaque pass

	FrameBufferCreateInfo opaqueFBCreateInfo = {};
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassColorOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassShadowOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassDepthOutput);
	opaqueFBCreateInfo.framebufferWidth = screenWidth;
	opaqueFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(opaqueFBCreateInfo, m_pOpaquePassFrameBuffer);

	// Frame buffer for Gaussian blur pass

	FrameBufferCreateInfo blurFBCreateInfo = {};
	blurFBCreateInfo.attachments.emplace_back(m_pBlurPassHorizontalColorOutput);
	blurFBCreateInfo.attachments.emplace_back(m_pBlurPassFinalColorOutput);
	blurFBCreateInfo.framebufferWidth = screenWidth;
	blurFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(blurFBCreateInfo, m_pBlurPassFrameBuffer);

	// Frame buffer for line drawing pass

	FrameBufferCreateInfo lineDrawFBCreateInfo = {};
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo.attachments.emplace_back(m_pLineDrawingPassBlurredOutput);
	lineDrawFBCreateInfo.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo, m_pLineDrawingPassFrameBuffer);

	// Frame buffer for transparent pass

	FrameBufferCreateInfo transpFBCreateInfo = {};
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassColorOutput);
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassDepthOutput);
	transpFBCreateInfo.framebufferWidth = screenWidth;
	transpFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(transpFBCreateInfo, m_pTranspPassFrameBuffer);

	// Frame buffer for blend pass

	FrameBufferCreateInfo blendFBCreateInfo = {};
	blendFBCreateInfo.attachments.emplace_back(m_pBlendPassColorOutput);
	blendFBCreateInfo.framebufferWidth = screenWidth;
	blendFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(blendFBCreateInfo, m_pBlendPassFrameBuffer);

	// Frame buffer for DOF pass

	FrameBufferCreateInfo dofFBCreateInfo = {};
	dofFBCreateInfo.attachments.emplace_back(m_pDOFPassHorizontalOutput);
	dofFBCreateInfo.framebufferWidth = screenWidth;
	dofFBCreateInfo.framebufferHeight = screenHeight;

	m_pDevice->CreateFrameBuffer(dofFBCreateInfo, m_pDOFPassFrameBuffer);
}

void ForwardRenderer::CreateUniformBuffers()
{
	UniformBufferCreateInfo ubCreateInfo = {};

	ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pLightSpaceTransformMatrix_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pMaterialNumericalProperties_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraPropertie_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBControlVariables);
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pControlVariables_UB);
}

void ForwardRenderer::BuildShadowMapPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_SHADOWMAP, m_pShadowMapPassFrameBuffer);
	passInput.Add(ForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(ForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);

	passOutput.Add(ForwardGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);

	auto pShadowMapPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto screenWidth = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
			auto screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Change viewport to shadow map size
			pDevice->ResizeViewPort(4096, 4096); // SHADOW_MAP_RESOLUTION

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_SHADOWMAP)));
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);

			UBTransformMatrices ubTransformMatrices;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));

			ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
			pLightSpaceTransformMatrixUB->UpdateBufferData(&ubLightSpaceTransformMatrix);

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

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

				auto subMeshes = pMesh->GetSubMeshes();
				for (unsigned int i = 0; i < subMeshes->size(); ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, pLightSpaceTransformMatrixUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
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

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_SHADOWMAP, pShadowMapPass);
}

void ForwardRenderer::BuildNormalOnlyPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_NORMALONLY, m_pNormalOnlyPassFrameBuffer);
	passInput.Add(ForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);

	passOutput.Add(ForwardGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passOutput.Add(ForwardGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);

	auto pNormalOnlyPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
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
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_NORMALONLY)));
			pDevice->ClearRenderTarget();

			// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);

			UBTransformMatrices ubTransformMatrices;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_TRANSFORM_MATRICES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

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

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);

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

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_NORMALONLY, pNormalOnlyPass);
}

void ForwardRenderer::BuildOpaquePass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_OPAQUE, m_pOpaquePassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passInput.Add(ForwardGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);
	passInput.Add(ForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(ForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);
	passInput.Add(ForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(ForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(ForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passOutput.Add(ForwardGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passOutput.Add(ForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	auto pOpaquePass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
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

			Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto pShadowMap = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_SHADOWMAP_DEPTH));

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_OPAQUE)));
			pDevice->ClearRenderTarget();

			UBTransformMatrices ubTransformMatrices;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			UBCameraProperties ubCameraProperties;
			UBMaterialNumericalProperties ubMaterialNumericalProperties;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CAMERA_PROPERTIES));
			auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
			pLightSpaceTransformMatrixUB->UpdateBufferData(&ubLightSpaceTransformMatrix);

			ubCameraProperties.cameraPosition = cameraPos;
			pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

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

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

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

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EDescriptorType::CombinedImageSampler, pShadowMap);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::UniformBuffer, pLightSpaceTransformMatrixUB);

					ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
					ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
					ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();
					pMaterialNumericalPropertiesUB->UpdateBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, pMaterialNumericalPropertiesUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
					if (pToneTexture)
					{
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TONE_TEXTURE), EDescriptorType::CombinedImageSampler, pToneTexture);
					}

					auto gNormalTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_NORMALONLY_NORMAL));
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler, gNormalTexture);

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);

					pShaderProgram->Reset();
				}
			
			}
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_OPAQUE, pOpaquePass);
}

void ForwardRenderer::BuildGaussianBlurPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_BLUR, m_pBlurPassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(ForwardGraphRes::TX_BLUR_HORIZONTAL_COLOR, m_pBlurPassHorizontalColorOutput);
	passInput.Add(ForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(ForwardGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);

	auto pGaussianBlurPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CONTROL_VARIABLES));

			// Horizontal subpass

			// Set render target to color attachment 0
			std::vector<uint32_t> attachmentIndex = { 0 };
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_BLUR)), attachmentIndex); // Alert: this won't work for Vulkan
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			auto colorTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_OPAQUE_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture_1);

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Vertical subpass

			// Set render target to color attachment 1
			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_BLUR)), attachmentIndex);

			pShaderParamTable->Clear();

			auto colorTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_BLUR_HORIZONTAL_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture_2);

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_BLUR, pGaussianBlurPass);
}

void ForwardRenderer::BuildLineDrawingPass()
{
	CreateLineDrawingMatrices(); // Pre-calculate local parameterization matrices

	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_LINEDRAWING, m_pLineDrawingPassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);
	passInput.Add(ForwardGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);	
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE, m_pLineDrawingMatrixTexture);
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_CURVATURE, m_pLineDrawingPassCurvatureOutput);
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_BLURRED, m_pLineDrawingPassBlurredOutput);
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(ForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(ForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);

	auto pLineDrawingPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();

			// Configure blend state
			DeviceBlendStateInfo blendInfo = {};
			blendInfo.enabled = false;
			pDevice->SetBlendState(blendInfo);

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CONTROL_VARIABLES));

			// Curvature computation subpass

			// Set render target
			std::vector<uint32_t> attachmentIndex = { 0 };
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_LINEDRAWING)), attachmentIndex); // Alert: this won't work for Vulkan
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			auto colorTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_BLUR_FINAL_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture_1);

			auto matrixTex = std::static_pointer_cast<RenderTexture>(input.Get(ForwardGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SAMPLE_MATRIX_TEXTURE), EDescriptorType::CombinedImageSampler, matrixTex);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Ridge searching subpass

			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_LINEDRAWING)), attachmentIndex);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
			pShaderParamTable->Clear();

			auto curvatureTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_LINEDRAWING_CURVATURE));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, curvatureTexture);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Gaussian blur subpasses

			// Horizontal subpass

			// Set render target to curvature color attachment
			attachmentIndex[0] = 0;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_LINEDRAWING)), attachmentIndex);
			pDevice->ClearRenderTarget();

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			pShaderParamTable->Clear();

			auto colorTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_LINEDRAWING_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture);

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Vertical subpass

			// Set render target to blurred color attachment
			attachmentIndex[0] = 2;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_LINEDRAWING)), attachmentIndex);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, curvatureTexture);

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

			// Final blend subpass

			// Set render target to color attachment
			attachmentIndex[0] = 1;
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_LINEDRAWING)), attachmentIndex);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
			pShaderParamTable->Clear();

			auto blurredTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_LINEDRAWING_BLURRED));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, blurredTexture);

			auto colorTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_OPAQUE_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, colorTexture_2);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();

		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_LINEDRAWING, pLineDrawingPass);
}

void ForwardRenderer::BuildTransparentPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_TRANSP, m_pTranspPassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(ForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(ForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(ForwardGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(ForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(ForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(ForwardGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passOutput.Add(ForwardGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pTransparentPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
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
		blendInfo.enabled = true;
		blendInfo.srcFactor = EBlendFactor::SrcAlpha;
		blendInfo.dstFactor = EBlendFactor::OneMinusSrcAlpha;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_TRANSP)));
		pDevice->ClearRenderTarget();

		UBTransformMatrices ubTransformMatrices;
		UBSystemVariables ubSystemVariables;
		UBCameraProperties ubCameraProperties;
		UBMaterialNumericalProperties ubMaterialNumericalProperties;
		auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_TRANSFORM_MATRICES));
		auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_SYSTEM_VARIABLES));
		auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CAMERA_PROPERTIES));
		auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

		ubTransformMatrices.projectionMatrix = projectionMat;
		ubTransformMatrices.viewMatrix = viewMat;

		ubSystemVariables.timeInSec = Timer::Now();
		pSystemVariablesUB->UpdateBufferData(&ubSystemVariables);

		ubCameraProperties.cameraPosition = cameraPos;
		pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

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

			ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
			ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
			pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);
	
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

				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, pSystemVariablesUB);

				ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
				pMaterialNumericalPropertiesUB->UpdateBufferData(&ubMaterialNumericalProperties);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::UniformBuffer, pMaterialNumericalPropertiesUB);

				auto depthTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_OPAQUE_DEPTH));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, depthTexture);

				auto colorTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_LINEDRAWING_COLOR));
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture);

				auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
				if (pAlbedoTexture)
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
				}

				auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
				if (pNoiseTexture)
				{
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
				}

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
				pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex);

				pShaderProgram->Reset();
			}
			
		}
	},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_TRANSP, pTransparentPass);
}

void ForwardRenderer::BuildOpaqueTranspBlendPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_BLEND, m_pBlendPassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(ForwardGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(ForwardGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passInput.Add(ForwardGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	passOutput.Add(ForwardGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);

	auto pBlendPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
	{
		auto pDevice = pContext->pRenderer->GetDrawingDevice();

		// Configure blend state
		DeviceBlendStateInfo blendInfo = {};
		blendInfo.enabled = false;
		pDevice->SetBlendState(blendInfo);

		// Set render target
		pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_BLEND)));
		pDevice->ClearRenderTarget();

		auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		auto depthTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_OPAQUE_DEPTH));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, depthTexture_1);
		
		auto colorTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_LINEDRAWING_COLOR));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture_1);

		auto depthTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_TRANSP_DEPTH));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_2), EDescriptorType::CombinedImageSampler, depthTexture_2);

		auto colorTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_TRANSP_COLOR));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, colorTexture_2);

		pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
		pDevice->DrawFullScreenQuad();

		pShaderProgram->Reset();
	},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_COLORBLEND_D2, pBlendPass);
}

void ForwardRenderer::BuildDepthOfFieldPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(ForwardGraphRes::FB_DOF, m_pDOFPassFrameBuffer);
	passInput.Add(ForwardGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);
	passInput.Add(ForwardGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);
	passInput.Add(ForwardGraphRes::TX_DOF_HORIZONTAL, m_pDOFPassHorizontalOutput);
	passInput.Add(ForwardGraphRes::TX_BRUSH_MASK_TEXTURE_1, m_pBrushMaskImageTexture_1);
	passInput.Add(ForwardGraphRes::TX_BRUSH_MASK_TEXTURE_2, m_pBrushMaskImageTexture_2);
	passInput.Add(ForwardGraphRes::TX_PENCIL_MASK_TEXTURE_1, m_pPencilMaskImageTexture_1);
	passInput.Add(ForwardGraphRes::TX_PENCIL_MASK_TEXTURE_2, m_pPencilMaskImageTexture_2);
	passInput.Add(ForwardGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passInput.Add(ForwardGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(ForwardGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(ForwardGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(ForwardGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	auto pDOFPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
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

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBCameraProperties ubCameraProperties;
			UBControlVariables ubControlVariables;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_SYSTEM_VARIABLES));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CAMERA_PROPERTIES));
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(ForwardGraphRes::UB_CONTROL_VARIABLES));

			ubTransformMatrices.viewMatrix = viewMat;
			pTransformMatricesUB->UpdateBufferData(&ubTransformMatrices);

			ubSystemVariables.timeInSec = Timer::Now();
			pSystemVariablesUB->UpdateBufferData(&ubSystemVariables);

			ubCameraProperties.aperture = pCameraComp->GetAperture();
			ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
			ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
			pCameraPropertiesUB->UpdateBufferData(&ubCameraProperties);

			// Horizontal subpass

			// Set render target
			pDevice->SetRenderTarget(std::static_pointer_cast<FrameBuffer>(input.Get(ForwardGraphRes::FB_DOF)));
			pDevice->ClearRenderTarget();

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::UniformBuffer, pTransformMatricesUB);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::UniformBuffer, pCameraPropertiesUB);

			auto colorTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_BLEND_COLOR));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, colorTexture_1);

			auto gPositionTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_NORMALONLY_POSITION));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler, gPositionTexture);

			ubControlVariables.bool_1 = 1;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			// Vertical subpass

			// Set to direct output
			pDevice->SetRenderTarget(nullptr); // Alert: this won't work for Vulkan

			pShaderParamTable->Clear();

			auto horizontalResultTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_DOF_HORIZONTAL));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, horizontalResultTexture);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::UniformBuffer, pSystemVariablesUB);

			auto brushMaskTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_BRUSH_MASK_TEXTURE_1));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler, brushMaskTexture_1);

			auto brushMaskTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_BRUSH_MASK_TEXTURE_2));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, brushMaskTexture_2);

			auto pencilMaskTexture_1 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_PENCIL_MASK_TEXTURE_1));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler, pencilMaskTexture_1);

			auto pencilMaskTexture_2 = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_PENCIL_MASK_TEXTURE_2));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler, pencilMaskTexture_2);

			auto shadowResultTexture = std::static_pointer_cast<Texture2D>(input.Get(ForwardGraphRes::TX_OPAQUE_SHADOW));
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, shadowResultTexture);

			ubControlVariables.bool_1 = 0;
			pControlVariablesUB->UpdateBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::UniformBuffer, pControlVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
			pDevice->DrawFullScreenQuad();

			pShaderProgram->Reset();
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(ForwardGraphRes::NODE_DOF, pDOFPass);
}

void ForwardRenderer::BuildRenderNodeDependencies()
{
	auto pShadowMapPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_SHADOWMAP);
	auto pNormalOnlyPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_NORMALONLY);
	auto pOpaquePass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_OPAQUE);
	auto pGaussianBlurPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_BLUR);
	auto pLineDrawingPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_LINEDRAWING);
	auto pTransparentPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_TRANSP);
	auto pBlendPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_COLORBLEND_D2);
	auto pDOFPass = m_pRenderGraph->GetNodeByName(ForwardGraphRes::NODE_DOF);

	pShadowMapPass->AddNextNode(pOpaquePass);
	pNormalOnlyPass->AddNextNode(pOpaquePass);
	pOpaquePass->AddNextNode(pGaussianBlurPass);
	pGaussianBlurPass->AddNextNode(pLineDrawingPass);
	pLineDrawingPass->AddNextNode(pTransparentPass);
	pTransparentPass->AddNextNode(pBlendPass);
	pBlendPass->AddNextNode(pDOFPass);
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