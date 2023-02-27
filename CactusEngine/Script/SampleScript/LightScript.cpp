#include "LightScript.h"
#include "AllComponents.h"
#include "BaseEntity.h"
#include "Timer.h"

using namespace Engine;

namespace SampleScript
{
	LightScript::LightScript(const std::shared_ptr<BaseEntity> pEntity)
		: m_pEntity(pEntity), m_pLightTransform(nullptr)
	{
		DEBUG_ASSERT_CE(pEntity != nullptr);
		m_id = EScriptID::Light;
	}

	void LightScript::Start()
	{
		m_pLightTransform = std::static_pointer_cast<TransformComponent>(m_pEntity->GetComponent(EComponentType::Transform));
		DEBUG_ASSERT_CE(m_pLightTransform != nullptr);

		m_center = m_pLightTransform->GetPosition();
		m_startTime = Timer::Now();

		m_started = true;
	}

	void LightScript::Update()
	{
		float elapsedTime = Timer::Now() - m_startTime;
		m_pLightTransform->SetPosition(m_center + Vector3(std::sinf(elapsedTime + m_center.x), 0.0f, std::cosf(elapsedTime + m_center.z)));
	}
}