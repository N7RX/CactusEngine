#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"

namespace SampleScript
{
	class CubeScript : public Engine::BaseScript
	{
	public:
		CubeScript(Engine::BaseEntity* pEntity);
		~CubeScript() = default;

		void Start() override;
		void Update() override;

	private:
		Engine::BaseEntity* m_pEntity;

		Engine::TransformComponent* m_pCubeTransform;
		Engine::MaterialComponent* m_pCubeMaterial;

		static uint32_t m_instanceCounter;
		uint32_t m_instanceIndex;
	};
}