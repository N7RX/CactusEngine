#pragma once
#include <cstdint>

namespace Engine
{
	__interface IApplication
	{
		void SetApplicationID(uint32_t id);
		uint32_t GetApplicationID() const;

		void Initialize();
		void Tick();
		void ShutDown();

		bool ShouldQuit() const;
	};
}