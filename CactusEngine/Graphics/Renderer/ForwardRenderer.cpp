#include "ForwardRenderer.h"
#include "TransformComponent.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "CameraComponent.h"
#include "DrawingSystem.h"

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

		Vector3 rotate = pTransformComp->GetRotation();
		Matrix4x4 modelMat = 
			glm::translate(pTransformComp->GetPosition())
			* (glm::rotate(rotate.x * D2R, X_AXIS)*glm::rotate(rotate.y * D2R, Y_AXIS)*glm::rotate(rotate.z * D2R, Z_AXIS))
			* glm::scale(pTransformComp->GetScale());
		Matrix4x4 pvmMatrix = projectionMat * viewMat * modelMat;

		auto pShaderProgram = m_pSystem->GetShaderProgramByType(pMaterialComp->GetShaderProgramType());
		auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::PVM_MATRIX), eShaderParam_Mat4, glm::value_ptr(pvmMatrix));
		pShaderParamTable->AddEntry(pShaderProgram->GetParamLocation(ShaderParamNames::ALBEDO_COLOR), eShaderParam_Vec4, glm::value_ptr(pMaterialComp->GetAlbedoColor()));

		m_pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable);
		m_pDevice->SetVertexBuffer(pMesh->GetVertexBuffer());
		m_pDevice->DrawPrimitive(pMesh->GetVertexBuffer()->GetNumberOfIndices());
	}
}