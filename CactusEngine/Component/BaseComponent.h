#pragma once
#include "IComponent.h"
#include "BasicMathTypes.h"

namespace Engine
{
	class BaseComponent : public IComponent
	{
	public:
		virtual ~BaseComponent() = default;

		void SetComponentID(uint32_t id);
		uint32_t GetComponentID() const;

		EComponentType GetComponentType() const;

		IEntity* GetParentEntity() const;
		void SetParentEntity(IEntity* pEntity);

	protected:
		BaseComponent(EComponentType type);

	protected:
		uint32_t m_componentID;
		EComponentType m_componentType;
		IEntity* m_pParentEntity;
	};
}