#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class BunnyScript : public Engine::BaseScript
	{
	public:
		BunnyScript(Engine::BaseEntity* pEntity);
		~BunnyScript() = default;

		void Start() override;
		void Update() override;

	private:
		Engine::BaseEntity* m_pEntity;

		Engine::TransformComponent* m_pBunnyTransform;

		static uint32_t m_instanceCounter;
		uint32_t m_instanceIndex;
	};
}