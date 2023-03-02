#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class LightScript : public Engine::BaseScript
	{
	public:
		LightScript(Engine::BaseEntity* pEntity);
		~LightScript() = default;

		void Start() override;
		void Update() override;

	private:
		Engine::BaseEntity* m_pEntity;

		Engine::TransformComponent* m_pLightTransform;
		Engine::Vector3 m_center;
		float m_startTime;
	};
}