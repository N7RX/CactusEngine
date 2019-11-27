#pragma once
#include "BaseScript.h"
#include "IEntity.h"

namespace SampleScript
{
	class CameraScript : public BaseScript
	{
	public:
		CameraScript(const std::shared_ptr<Engine::IEntity> pEntity);
		~CameraScript() = default;

		void Start() override;
		void Update() override;

	private:
		std::shared_ptr<Engine::IEntity> m_pEntity;
		Engine::Vector2 m_prevCursorPosition;
	};
}