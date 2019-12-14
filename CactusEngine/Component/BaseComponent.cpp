#include "BaseComponent.h"

using namespace Engine;

BaseComponent::BaseComponent(EComponentType type)
	: m_componentType(type), m_pParentEntity(nullptr), m_componentID(-1)
{
}

void BaseComponent::SetComponentID(uint32_t id)
{
	m_componentID = id;
}

uint32_t BaseComponent::GetComponentID() const
{
	return m_componentID;
}

EComponentType BaseComponent::GetComponentType() const
{
	return m_componentType;
}

IEntity* BaseComponent::GetParentEntity() const
{
	return m_pParentEntity;
}

void BaseComponent::SetParentEntity(IEntity* pEntity)
{
	assert(m_pParentEntity == nullptr);
	m_pParentEntity = pEntity;
}