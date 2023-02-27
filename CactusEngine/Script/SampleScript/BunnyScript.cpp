#include "BunnyScript.h"
#include "AllComponents.h"
#include "BaseEntity.h"
#include "InputSystem.h"
#include "Timer.h"
#include "LogUtility.h"

#include <algorithm>

using namespace Engine;

namespace SampleScript
{
	int BunnyScript::m_instanceCounter = 0;

	BunnyScript::BunnyScript(const std::shared_ptr<BaseEntity> pEntity)
		: m_pEntity(pEntity), m_pBunnyTransform(nullptr)
	{
		DEBUG_ASSERT_CE(pEntity != nullptr);

		m_id = EScriptID::Bunny;

		m_instanceIndex = m_instanceCounter;
		m_instanceCounter++;

		m_instanceCounter %= 2;
	}

	void BunnyScript::Start()
	{
		m_pBunnyTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(EComponentType::Transform));
		DEBUG_ASSERT_CE(m_pBunnyTransform != nullptr);

		m_started = true;
	}

	void BunnyScript::Update()
	{
		if (m_instanceIndex == 0)
		{
			static float startTime = Timer::Now();
			float deltaTime = Timer::Now() - startTime;
			Vector3 currPos = m_pBunnyTransform->GetPosition();
			currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f), 0.0f, 1.0f);
			m_pBunnyTransform->SetPosition(currPos);
		}
		else
		{
			static float startTime = Timer::Now();
			float deltaTime = Timer::Now() - startTime;
			Vector3 currPos = m_pBunnyTransform->GetPosition();
			currPos.y = std::clamp(1.0f * sinf(deltaTime * 4.0f + 3.1415926536f), 0.0f, 1.0f);
			m_pBunnyTransform->SetPosition(currPos);
		}
	}
}