#include "AnimationComponent.h"

using namespace Engine;

AnimationComponent::AnimationComponent()
	: BaseComponent(EComponentType::Animation)
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