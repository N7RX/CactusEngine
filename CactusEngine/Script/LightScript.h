#pragma once
#include "IScript.h"
#include "IEntity.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class LightScript : public Engine::IScript
	{
	public:
		LightScript(const std::shared_ptr<Engine::IEntity> pEntity);
		~LightScript() = default;

		EScriptID GetScriptID();
		bool ShouldCallStart();

		void Start();
		void Update();

	private:
		EScriptID m_id;
		bool m_started;
		std::shared_ptr<Engine::IEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pLightTransform;
		Engine::Vector3 m_center;
		float m_startTime;
	};
}