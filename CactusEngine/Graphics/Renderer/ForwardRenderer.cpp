#include "ForwardRenderer.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "CameraComponent.h"
#include "DrawingSystem.h"
#include "GLCDrawingSystem.h"

using namespace Engine;

ForwardRenderer::ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(eRenderer_Forward, pDevice, pSystem)
{
}

void ForwardRenderer::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pCamera->GetComponent(eCompType_Transform));
	auto pCameraComp = std::static_pointer_cast<CameraComponent>(pCamera->GetComponent(eCompType_Camera));
	if (!pCameraComp || !pCameraTransform)
	{
		return;
	}
	Matrix4x4 viewMat = glm::lookAt(pCameraTransform->GetPosition(), pCameraTransform->GetPosition() + pCameraTransform->GetForwardDirection(), UP);
	Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(), 
		float(gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowWidth()) / float(gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetWindowHeight()), 
		pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

	for (auto& entity : drawList)
	{
		auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(eCompType_Transform));
		auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(eCompType_MeshFilter));
		auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(eCompType_Material));

		if (!pTransformComp || !pMeshFilterComp || !pMaterialComp)
		{
			continue;
		}

		auto pMesh = pMeshFilterComp->GetMesh();

		if (!pMesh)
		{
			continue;
		}

		Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
		Matrix4x4 pvmMatrix = projectionMat * viewMat * modelMat;

		auto pAlbedoTexture = pMaterialComp->GetAlbedoTexture();

		auto pShaderProgram = m_pSystem->GetShaderProgramByType(pMaterialComp->GetShaderProgramType());
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PVM_MATRIX), eShaderParam_Mat4, glm::value_ptr(pvmMatrix));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), eShaderParam_Vec4, glm::value_ptr(pMaterialComp->GetAlbedoColor()));

		if (pAlbedoTexture)
		{
			// Alert: this only works for OpenGL; Vulkan requires doing this through descriptor sets
			uint32_t albedoTexID = pAlbedoTexture->GetTextureID();
			pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_TEXTURE), eShaderParam_Texture2D, &albedoTexID);
		}

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
		m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());
		m_pDevice->DrawPrimitive(pMesh->GetVertexBuffer()->GetNumberOfIndices());

		pShaderProgram->Reset();
	}
}