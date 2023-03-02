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

		static int m_instanceCounter;
		int m_instanceIndex;
	};
}