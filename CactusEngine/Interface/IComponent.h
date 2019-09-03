#pragma once
#include "SharedTypes.h"
#include <cstdint>
#include <memory>

namespace Engine
{
	__interface IEntity;
	__interface IComponent
	{
		void SetComponentID(uint32_t id);
		uint32_t GetComponentID() const;

		EComponentType GetComponentType() const;

		IEntity* GetParentEntity() const;
		void SetParentEntity(IEntity* pEntity);
	};
}