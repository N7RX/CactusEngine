#include "MeshFilterComponent.h"

namespace Engine
{
	MeshFilterComponent::MeshFilterComponent()
		: BaseComponent(EComponentType::MeshFilter)
	{
	}

	void MeshFilterComponent::SetMesh(Mesh* pMesh)
	{
		m_pMesh = pMesh;
	}

	Mesh* MeshFilterComponent::GetMesh() const
	{
		return m_pMesh;
	}
}