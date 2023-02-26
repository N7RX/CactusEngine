#include "StandardEntity.h"

namespace Engine
{
	StandardEntity::StandardEntity()
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
}