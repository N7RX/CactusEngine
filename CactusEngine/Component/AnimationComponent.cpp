#include "AnimationComponent.h"

using namespace Engine;

AnimationComponent::AnimationComponent()
	: BaseComponent(EComponentType::Animation)
{
}

void AnimationComponent::SetAnimFunction(void(*pAnimFunc)(IEntity* pEntity))
{
	m_pAnimFunc = pAnimFunc;
}

void AnimationComponent::Apply()
{
	m_pAnimFunc(m_pParentEntity);
}