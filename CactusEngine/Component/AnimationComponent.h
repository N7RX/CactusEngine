#pragma once
#include "BaseComponent.h"

namespace Engine
{
	class AnimationComponent : public BaseComponent
	{
	public:
		AnimationComponent();
		~AnimationComponent() = default;

		void SetAnimFunction(void(*pAnimFunc)(BaseEntity* pEntity));
		void Apply();

	private:
		void (*m_pAnimFunc)(BaseEntity* pEntity);
	};
}