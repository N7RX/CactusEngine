#include "ECSWorld.h"

using namespace Engine;

ECSWorld::ECSWorld()
{
	m_IDAssignments.resize(ECSTYPE_COUNT, 0);
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
	m_systemList.erase(m_systemList[type]->GetSystemID());
}

const EntityList* ECSWorld::GetEntityList() const
{
	return &m_entityList;
}

std::shared_ptr<IEntity> ECSWorld::FindEntityWithTag(EEntityTag tag) const
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

std::vector<std::shared_ptr<IEntity>> ECSWorld::FindEntitiesWithTag(EEntityTag tag) const
{
	std::vector<std::shared_ptr<IEntity>> result;
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
	assert(type < m_IDAssignments.size());
	return m_IDAssignments[type]++; // Alert: the ID may run out in rare cases
}