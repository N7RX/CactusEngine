#pragma once
#include "BaseEntity.h"
#include "BaseComponent.h"
#include "BaseSystem.h"
#include "MemoryAllocator.h"

namespace Engine
{
	typedef std::unordered_map<uint32_t, BaseEntity*> EntityList;
	typedef std::vector<BaseSystem*> SystemList;

	class ECSWorld
	{
	public:
		ECSWorld();
		virtual ~ECSWorld() = default;

		void Initialize();
		void ShutDown();

		void Tick();

		template<typename T>
		inline T* CreateEntity()
		{
			T* pEntity;
			CE_NEW(pEntity, T);
			pEntity->SetEntityID(GetNewECSID(EECSType::Entity));
			m_entityList.emplace(pEntity->GetEntityID(), pEntity);
			return pEntity;
		}

		template<typename T>
		inline T* CreateComponent()
		{
			T* pComponent;
			CE_NEW(pComponent, T);
			pComponent->SetComponentID(GetNewECSID(EECSType::Component));
			return pComponent;
		}

		template<typename T>
		inline void RegisterSystem(ESystemType type, uint32_t priority)
		{
			T* pSystem;
			CE_NEW(pSystem, T, this);
			pSystem->SetSystemID((uint32_t)type);
			pSystem->SetSystemPriority(priority);
			m_systemList.push_back(pSystem);
		}

		BaseSystem* GetSystem(ESystemType type) const;

		void RemoveEntity(uint32_t entityID);
		void RemoveSystem(ESystemType type);

		void SortSystems(); // Sort systems by priority. Should be called when new system is added

		const EntityList* GetEntityList() const;

		BaseEntity* FindEntityWithTag(EEntityTag tag) const;
		std::vector<BaseEntity*> FindEntitiesWithTag(EEntityTag tag) const;

		void ClearEntities();

	private:
		uint32_t GetNewECSID(EECSType type);

	private:
		EntityList m_entityList;
		SystemList m_systemList;

		std::vector<uint32_t> m_IDAssignments;
	};
}