#pragma once
#include "IScript.h"
#include "IEntity.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class BunnyScript : public Engine::IScript
	{
	public:
		BunnyScript(const std::shared_ptr<Engine::IEntity> pEntity);
		~BunnyScript() = default;

		EScriptID GetScriptID();
		bool ShouldCallStart();

		void Start();
		void Update();

	private:
		EScriptID m_id;
		bool m_started;
		std::shared_ptr<Engine::IEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pBunnyTransform;

		static int m_instanceCounter;
		int m_instanceIndex;
	};
}