#pragma once
#include "BaseSystem.h"
#include "ECSWorld.h"

namespace Engine
{
	class EventSystem : public BaseSystem
	{
	public:
		EventSystem(ECSWorld* pWorld) {};
		~EventSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

	private:
	};
}