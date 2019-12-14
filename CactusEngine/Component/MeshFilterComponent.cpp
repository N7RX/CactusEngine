#include "MeshFilterComponent.h"

using namespace Engine;

MeshFilterComponent::MeshFilterComponent()
	: BaseComponent(EComponentType::MeshFilter)
{
}

void MeshFilterComponent::SetMesh(const std::shared_ptr<Mesh> pMesh)
{
	m_pMesh = pMesh;
}

std::shared_ptr<Mesh> MeshFilterComponent::GetMesh() const
{
	return m_pMesh;
}