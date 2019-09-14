#pragma once
#include <chrono>

namespace Engine
{
	class Timer
	{
	public:
		static void Initialize();
		static void FrameBegin();
		static void FrameEnd();

		static long long TimeSinceStartUp(); // In nanoseconds
		static float Now();					 // In seconds
		static unsigned int GetCurrentFPS();
		static unsigned int GetAverageFPS(); // Average FPS in approximately 1 second
		static float GetFrameDeltaTime();	 // In seconds
	
	private:
		static std::chrono::time_point<std::chrono::high_resolution_clock> m_sTimeAtStartUp;
		static std::chrono::time_point<std::chrono::high_resolution_clock> m_sTimeAtBeginOfFrame;
		static float m_sFrameDeltaTime;
		static unsigned int m_sCurrentFPS;
		static unsigned int m_sAverageFPS;
		static std::chrono::duration<float, std::milli> m_sOneSecDuration;
		static unsigned int m_sOneSecTicks;
	};
}