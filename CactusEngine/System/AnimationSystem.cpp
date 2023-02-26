#include "AnimationSystem.h"
#include "AnimationComponent.h"

namespace Engine
{
	AnimationSystem::AnimationSystem(ECSWorld* pWorld)
		: m_pECSWorld(pWorld)
	{

	}

	void AnimationSystem::Initialize()
	{

	}

	void AnimationSystem::ShutDown()
	{

	}

	void AnimationSystem::FrameBegin()
	{

	}

	void AnimationSystem::Tick()
	{
		auto pEntityList = m_pECSWorld->GetEntityList();
		for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
		{
			auto pAnimationComp = std::static_pointer_cast<AnimationComponent>(itr->second->GetComponent(EComponentType::Animation));
			if (pAnimationComp)
			{
				pAnimationComp->Apply();
			}
		}
	}

	void AnimationSystem::FrameEnd()
	{

	}
}