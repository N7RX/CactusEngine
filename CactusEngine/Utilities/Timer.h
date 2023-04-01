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

		static int64_t TimeSinceStartUp(); // In nanoseconds
		static float Now();				   // In seconds
		static uint32_t GetCurrentFPS();
		static uint32_t GetAverageFPS();   // Average FPS in approximately 1 second
		static float GetFrameDeltaTime();  // In seconds
		static uint64_t GetCurrentFrame();

	private:
		static std::chrono::time_point<std::chrono::high_resolution_clock> m_sTimeAtStartUp;
		static std::chrono::time_point<std::chrono::high_resolution_clock> m_sTimeAtBeginOfFrame;
		static float m_sFrameDeltaTime;
		static uint32_t m_sCurrentFPS;
		static uint32_t m_sAverageFPS;
		static std::chrono::duration<float, std::milli> m_sOneSecDuration;
		static uint32_t m_sOneSecTicks;
		static uint64_t m_frameCount;
	};
}