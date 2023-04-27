#pragma once
#include "BaseSystem.h"
#include "ECSWorld.h"

namespace Engine
{
	class AudioSystem : public BaseSystem
	{
	public:
		AudioSystem(ECSWorld* pWorld) {};
		~AudioSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

	private:
	};
}