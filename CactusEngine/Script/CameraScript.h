#pragma once
#include "IScript.h"
#include "IEntity.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class CameraScript : public Engine::IScript
	{
	public:
		CameraScript(const std::shared_ptr<Engine::IEntity> pEntity);
		~CameraScript() = default;

		EScriptID GetScriptID();
		bool ShouldCallStart();

		void Start();
		void Update();

		float m_cameraRotateSpeed = 20.0f;
		float m_cameraMoveSpeed = 3.0f;

	private:
		EScriptID m_id;
		bool m_started;
		std::shared_ptr<Engine::IEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pCameraTransform;
		Engine::Vector2 m_prevCursorPosition;
	};
}