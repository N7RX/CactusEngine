#include "ECSWorld.h"
#include "LogUtility.h"

#include <algorithm>

namespace Engine
{
	ECSWorld::ECSWorld()
	{
		m_IDAssignments.resize((size_t)EECSType::COUNT, 0);
	}

	void ECSWorld::Initialize()
	{
		for (auto& system : m_systemList)
		{
			system->Initialize();
		}
	}

	void ECSWorld::ShutDown()
	{
		for (auto& system : m_systemList)
		{
			system->ShutDown();
		}
	}

	void ECSWorld::Tick()
	{
		for (auto& system : m_systemList)
		{
			system->FrameBegin();
		}

		for (auto& system : m_systemList)
		{
			system->Tick();
		}

		for (auto& system : m_systemList)
		{
			system->FrameEnd();
		}
	}

	BaseSystem* ECSWorld::GetSystem(ESystemType type) const
	{
		for (auto& system : m_systemList)
		{
			if (system->GetSystemID() == (uint32_t)type)
			{
				return system;
			}
		}

		LOG_ERROR("Requested system does not exist in current environment.");
		return nullptr;
	}

	void ECSWorld::RemoveEntity(uint32_t entityID)
	{
		m_entityList.erase(entityID);
	}

	void ECSWorld::RemoveSystem(ESystemType type)
	{
		for (auto itr = m_systemList.begin(); itr != m_systemList.end(); ++itr)
		{
			if ((*itr)->GetSystemID() == (uint32_t)type)
			{
				m_systemList.erase(itr);
				return;
			}
		}
		DEBUG_LOG_ERROR("The system requested to remove does not exist.");
	}

	void ECSWorld::SortSystems()
	{
		std::sort(m_systemList.begin(), m_systemList.end(), [](const BaseSystem* lhs, const BaseSystem* rhs)
			{
				return lhs->GetSystemPriority() < rhs->GetSystemPriority();
			});
	}

	const EntityList* ECSWorld::GetEntityList() const
	{
		return &m_entityList;
	}

	BaseEntity* ECSWorld::FindEntityWithTag(EEntityTag tag) const
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

	std::vector<BaseEntity*> ECSWorld::FindEntitiesWithTag(EEntityTag tag) const
	{
		std::vector<BaseEntity*> result;
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
		DEBUG_ASSERT_CE((uint32_t)type < m_IDAssignments.size());
		return m_IDAssignments[(uint32_t)type]++;
	}
}