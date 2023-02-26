#pragma once
#include "BaseEntity.h"
#include "BaseComponent.h"
#include "BaseSystem.h"

namespace Engine
{
	typedef std::unordered_map<uint32_t, std::shared_ptr<BaseEntity>> EntityList;
	typedef std::unordered_map<uint32_t, std::shared_ptr<BaseSystem>> SystemList;

	class ECSWorld : std::enable_shared_from_this<ECSWorld>
	{
	public:
		ECSWorld();
		virtual ~ECSWorld() = default;

		void Initialize();
		void ShutDown();

		void Tick();

		template<typename T>
		inline std::shared_ptr<T> CreateEntity()
		{
			auto pEntity = std::make_shared<T>();
			pEntity->SetEntityID(GetNewECSID(EECSType::Entity));
			m_entityList.emplace(pEntity->GetEntityID(), pEntity);
			return pEntity;
		}

		template<typename T>
		inline std::shared_ptr<T> CreateComponent()
		{
			auto pComponent = std::make_shared<T>();
			pComponent->SetComponentID(GetNewECSID(EECSType::Component));
			return pComponent;
		}

		template<typename T>
		inline void RegisterSystem(ESystemType type)
		{
			auto pSystem = std::make_shared<T>(this);
			pSystem->SetSystemID((uint32_t)type);
			m_systemList.emplace((uint32_t)type, pSystem);
		}

		void RemoveEntity(uint32_t entityID);
		void RemoveSystem(ESystemType type);

		const EntityList* GetEntityList() const;

		std::shared_ptr<BaseEntity> FindEntityWithTag(EEntityTag tag) const;
		std::vector<std::shared_ptr<BaseEntity>> FindEntitiesWithTag(EEntityTag tag) const;

		void ClearEntities();

	private:
		uint32_t GetNewECSID(EECSType type);

	private:
		EntityList m_entityList;
		SystemList m_systemList;

		std::vector<uint32_t> m_IDAssignments;
	};
}