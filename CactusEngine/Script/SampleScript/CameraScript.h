#pragma once
#include "BaseScript.h"
#include "TransformComponent.h"

namespace SampleScript
{
	class CameraScript : public Engine::BaseScript
	{
	public:
		CameraScript(const std::shared_ptr<Engine::BaseEntity> pEntity);
		~CameraScript() = default;

		void Start() override;
		void Update() override;

		float m_cameraRotateSpeed = 20.0f;
		float m_cameraMoveSpeed = 3.0f;

	private:
		std::shared_ptr<Engine::BaseEntity> m_pEntity;

		std::shared_ptr<Engine::TransformComponent> m_pCameraTransform;
		Engine::Vector2 m_prevCursorPosition;
	};
}