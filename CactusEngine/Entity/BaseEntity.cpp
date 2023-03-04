#include "BaseEntity.h"
#include "BaseComponent.h"

namespace Engine
{
	BaseEntity::BaseEntity()
		: m_componentBitmap(0),
		m_tag(EEntityTag::None),
		m_entityID(-1)
	{
	}

	uint32_t BaseEntity::GetEntityID() const
	{
		return m_entityID;
	}

	void BaseEntity::SetEntityID(uint32_t id)
	{
		m_entityID = id;
	}

	void BaseEntity::AttachComponent(BaseComponent* pComponent)
	{
		m_componentList.emplace(pComponent->GetComponentType(), pComponent);
		m_componentBitmap |= (uint32_t)pComponent->GetComponentType();
		pComponent->SetParentEntity(this);
	}

	void BaseEntity::DetachComponent(EComponentType compType)
	{
		if ((m_componentBitmap & (uint32_t)compType) == (uint32_t)compType)
		{
			m_componentList.at(compType)->SetParentEntity(nullptr);
			m_componentList.erase(compType);
			m_componentBitmap ^= (uint32_t)compType;
		}
	}

	const ComponentList& BaseEntity::GetComponentList() const
	{
		return m_componentList;
	}

	BaseComponent* BaseEntity::GetComponent(EComponentType compType) const
	{
		if ((m_componentBitmap & (uint32_t)compType) == (uint32_t)compType)
		{
			return m_componentList.at(compType);
		}
		return nullptr;
	}

	EEntityTag BaseEntity::GetEntityTag() const
	{
		return m_tag;
	}

	void BaseEntity::SetEntityTag(EEntityTag tag)
	{
		m_tag = tag;
	}
}