#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"

namespace SampleScript
{
	class CubeScript : public Engine::BaseScript
	{
	public:
		CubeScript(const std::shared_ptr<Engine::BaseEntity> pEntity);
		~CubeScript() = default;

		void Start() override;
		void Update() override;

	private:
		std::shared_ptr<Engine::BaseEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pCubeTransform;
		std::shared_ptr<Engine::MaterialComponent> m_pCubeMaterial;

		static int m_instanceCounter;
		int m_instanceIndex;
	};
}