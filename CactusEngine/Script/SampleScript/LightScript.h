#pragma once
#include "BaseScript.h"
#include "BaseEntity.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class LightScript : public Engine::BaseScript
	{
	public:
		LightScript(const std::shared_ptr<Engine::BaseEntity> pEntity);
		~LightScript() = default;

		void Start() override;
		void Update() override;

	private:
		std::shared_ptr<Engine::BaseEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pLightTransform;
		Engine::Vector3 m_center;
		float m_startTime;
	};
}