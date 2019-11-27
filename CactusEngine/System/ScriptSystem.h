#pragma once
#include "ISystem.h"
#include "Global.h"
#include "ECSWorld.h"
#include "NoCopy.h"
#include <chrono>

namespace Engine
{
	class ScriptSystem : public ISystem, public NoCopy
	{
	public:
		ScriptSystem(ECSWorld* pWorld);
		~ScriptSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

	private:
		uint32_t m_systemID;
		ECSWorld* m_pECSWorld;
	};
}