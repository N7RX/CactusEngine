#pragma once
#include "SharedTypes.h"
#include "EntityProperties.h"
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace Engine
{
	__interface IComponent;
	typedef std::unordered_map<EComponentType, std::shared_ptr<IComponent>> ComponentList;

	__interface IEntity
	{
		void SetEntityID(uint32_t id);
		uint32_t GetEntityID() const;

		void AttachComponent(const std::shared_ptr<IComponent> pComponent);
		void DetachComponent(EComponentType compType);

		const ComponentList& GetComponentList() const;
		std::shared_ptr<IComponent> GetComponent(EComponentType compType) const;

		EEntityTag GetEntityTag() const;
		void SetEntityTag(EEntityTag tag);
	};
}