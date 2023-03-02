#pragma once
#include "BasicMathTypes.h"
#include "SharedTypes.h"

namespace Engine
{
	class BaseEntity;

	class BaseComponent
	{
	public:
		virtual ~BaseComponent() = default;

		void SetComponentID(uint32_t id);
		uint32_t GetComponentID() const;

		EComponentType GetComponentType() const;

		BaseEntity* GetParentEntity() const;
		void SetParentEntity(BaseEntity* pEntity);

	protected:
		BaseComponent(EComponentType type);

	protected:
		uint32_t m_componentID;
		EComponentType m_componentType;
		BaseEntity* m_pParentEntity;
	};
}