#include "StandardEntity.h"
#include "MeshFilterComponent.h"
#include "LightComponent.h"

namespace Engine
{
	const uint32_t INVALID_DRAWCALL_COUNT = -1;

	StandardEntity::StandardEntity()
		: BaseEntity(),
		m_maxDrawCallCount(INVALID_DRAWCALL_COUNT)
	{
		m_entityName = "Entity";
	}

	void StandardEntity::SetEntityName(const char* name)
	{
		m_entityName = name;
	}

	const char* StandardEntity::GetEntityName() const
	{
		return m_entityName;
	}

	uint32_t StandardEntity::EstimateMaxDrawCallCount()
	{
		// Alert: we need to invalidate the estimation if the mesh is changed
		if (m_maxDrawCallCount == INVALID_DRAWCALL_COUNT)
		{
			m_maxDrawCallCount = 0;

			auto pMeshFilterComp = GetComponent<MeshFilterComponent>(EComponentType::MeshFilter);
			if (pMeshFilterComp && pMeshFilterComp->GetMesh())
			{
				m_maxDrawCallCount = pMeshFilterComp->GetMesh()->GetSubmeshCount();
			}
			
			auto pLightComp = GetComponent<LightComponent>(EComponentType::Light);
			if (pLightComp && pLightComp->GetProfile().pVolumeMesh)
			{
				uint32_t lightMeshDrawCallCount = pLightComp->GetProfile().pVolumeMesh->GetSubmeshCount();

				if (lightMeshDrawCallCount > m_maxDrawCallCount)
				{
					m_maxDrawCallCount = lightMeshDrawCallCount;
				}
			}
		}
		return m_maxDrawCallCount;
	}
}