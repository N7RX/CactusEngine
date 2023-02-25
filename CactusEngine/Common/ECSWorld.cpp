#include "ECSWorld.h"

using namespace Engine;

ECSWorld::ECSWorld()
{
	m_IDAssignments.resize((size_t)EECSType::COUNT, 0);
}

void ECSWorld::Initialize()
{
	for (auto& system : m_systemList)
	{
		system.second->Initialize();
	}
}

void ECSWorld::ShutDown()
{
	for (auto& system : m_systemList)
	{
		system.second->ShutDown();
	}
}

void ECSWorld::Tick()
{
	for (auto& system : m_systemList)
	{
		system.second->FrameBegin();
	}

	for (auto& system : m_systemList)
	{
		system.second->Tick();
	}

	for (auto& system : m_systemList)
	{
		system.second->FrameEnd();
	}
}

void ECSWorld::RemoveEntity(uint32_t entityID)
{
	m_entityList.erase(entityID);
}

void ECSWorld::RemoveSystem(ESystemType type)
{
	m_systemList.erase(m_systemList[(uint32_t)type]->GetSystemID());
}

const EntityList* ECSWorld::GetEntityList() const
{
	return &m_entityList;
}

std::shared_ptr<BaseEntity> ECSWorld::FindEntityWithTag(EEntityTag tag) const
{
	for (auto entity : m_entityList)
	{
		if (entity.second->GetEntityTag() == tag)
		{
			return entity.second;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<BaseEntity>> ECSWorld::FindEntitiesWithTag(EEntityTag tag) const
{
	std::vector<std::shared_ptr<BaseEntity>> result;
	for (auto entity : m_entityList)
	{
		if (entity.second->GetEntityTag() == tag)
		{
			result.emplace_back(entity.second);
		}
	}
	return result;
}

void ECSWorld::ClearEntities()
{
	m_entityList.clear();
}

uint32_t ECSWorld::GetNewECSID(EECSType type)
{
	assert((uint32_t)type < m_IDAssignments.size());
	return m_IDAssignments[(uint32_t)type]++; // Alert: the ID may run out in rare cases
}