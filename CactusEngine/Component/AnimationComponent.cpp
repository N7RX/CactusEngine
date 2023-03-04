#include "AnimationComponent.h"

namespace Engine
{
	AnimationComponent::AnimationComponent()
		: BaseComponent(EComponentType::Animation),
		m_pAnimFunc(nullptr)
	{
	}

	void AnimationComponent::SetAnimFunction(void(*pAnimFunc)(BaseEntity* pEntity))
	{
		m_pAnimFunc = pAnimFunc;
	}

	void AnimationComponent::Apply()
	{
		m_pAnimFunc(m_pParentEntity);
	}
}