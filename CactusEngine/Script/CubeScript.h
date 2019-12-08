#pragma once
#include "IScript.h"
#include "IEntity.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"

namespace SampleScript
{
	class CubeScript : public Engine::IScript
	{
	public:
		CubeScript(const std::shared_ptr<Engine::IEntity> pEntity);
		~CubeScript() = default;

		EScriptID GetScriptID();
		bool ShouldCallStart();

		void Start();
		void Update();

	private:
		EScriptID m_id;
		bool m_started;
		std::shared_ptr<Engine::IEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pCubeTransform;
		std::shared_ptr<Engine::MaterialComponent> m_pCubeMaterial;

		static int m_instanceCounter;
		int m_instanceIndex;
	};
}