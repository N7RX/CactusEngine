#pragma once
#include "BaseComponent.h"

namespace Engine
{
	class AnimationComponent : public BaseComponent
	{
	public:
		AnimationComponent();
		~AnimationComponent() = default;

		void SetAnimFunction(void(*pAnimFunc)(IEntity* pEntity));
		void Apply();

	private:
		void (*m_pAnimFunc)(IEntity* pEntity);
	};
}