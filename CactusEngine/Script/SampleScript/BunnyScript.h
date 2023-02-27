#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class BunnyScript : public Engine::BaseScript
	{
	public:
		BunnyScript(const std::shared_ptr<Engine::BaseEntity> pEntity);
		~BunnyScript() = default;

		void Start() override;
		void Update() override;

	private:
		std::shared_ptr<Engine::BaseEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pBunnyTransform;

		static int m_instanceCounter;
		int m_instanceIndex;
	};
}