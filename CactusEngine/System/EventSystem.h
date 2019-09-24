#pragma once
#include "ISystem.h"
#include "ECSWorld.h"
#include "Global.h"

namespace Engine
{
	class EventSystem : public ISystem
	{
	public:
		EventSystem(ECSWorld* pWorld) {};
		~EventSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

	private:
		uint32_t m_systemID;
	};
}