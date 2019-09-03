#pragma once
#include "SharedTypes.h"
#include <cstdint>

namespace Engine
{
	__interface ISystem
	{
		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();
	};
}