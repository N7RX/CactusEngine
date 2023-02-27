#pragma once
#include "BaseSystem.h"
#include "ECSWorld.h"

#include <chrono>

namespace Engine
{
	class ScriptSystem : public BaseSystem
	{
	public:
		ScriptSystem(ECSWorld* pWorld);
		~ScriptSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

	private:
		ECSWorld* m_pECSWorld;
	};
}