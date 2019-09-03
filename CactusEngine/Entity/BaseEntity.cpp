#include "BaseEntity.h"

using namespace Engine;

BaseEntity::BaseEntity()
	: m_componentBitmap(0), m_tag(eEntityTag_None)
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

void BaseEntity::AttachComponent(const std::shared_ptr<IComponent> pComponent)
{
	m_componentList.emplace(pComponent->GetComponentType(), pComponent);
	m_componentBitmap |= pComponent->GetComponentType();
	pComponent->SetParentEntity(this);
}

void BaseEntity::DetachComponent(EComponentType compType)
{
	if ((m_componentBitmap & compType) == compType)
	{
		m_componentList.at(compType)->SetParentEntity(nullptr);
		m_componentList.erase(compType);
		m_componentBitmap ^= compType;
	}
}

const ComponentList& BaseEntity::GetComponentList() const
{
	return m_componentList;
}

std::shared_ptr<IComponent> BaseEntity::GetComponent(EComponentType compType) const
{
	if ((m_componentBitmap & compType) == compType)
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