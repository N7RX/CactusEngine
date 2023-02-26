#include "Timer.h"

namespace Engine
{
	std::chrono::time_point<std::chrono::high_resolution_clock> Timer::m_sTimeAtStartUp;
	std::chrono::time_point<std::chrono::high_resolution_clock> Timer::m_sTimeAtBeginOfFrame;
	float Timer::m_sFrameDeltaTime = 0.0f;
	unsigned int Timer::m_sCurrentFPS = 0;
	unsigned int Timer::m_sAverageFPS = 0;
	std::chrono::duration<float, std::milli> Timer::m_sOneSecDuration;
	unsigned int Timer::m_sOneSecTicks = 0;
	uint64_t Timer::m_frameCount = 0;

	void Timer::Initialize()
	{
		m_sTimeAtStartUp = std::chrono::high_resolution_clock::now();
		m_sTimeAtBeginOfFrame = m_sTimeAtStartUp;
	}

	void Timer::FrameBegin()
	{
		m_sTimeAtBeginOfFrame = std::chrono::high_resolution_clock::now();
		m_frameCount++;
	}

	void Timer::FrameEnd()
	{
		auto lastFrameInterval = std::chrono::high_resolution_clock::now() - m_sTimeAtBeginOfFrame;
		auto lastFrameDuration = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(lastFrameInterval);
		m_sFrameDeltaTime = lastFrameDuration.count() / 1000.0f;
		m_sCurrentFPS = (unsigned int)(1000.0f / lastFrameDuration.count());

		m_sOneSecDuration += lastFrameDuration;
		if (m_sOneSecDuration.count() >= 1000.0f)
		{
			m_sAverageFPS = m_sOneSecTicks;
			m_sOneSecTicks = 0;
			m_sOneSecDuration = std::chrono::milliseconds::zero();
		}

		m_sOneSecTicks++;
	}

	long long Timer::TimeSinceStartUp()
	{
		return (std::chrono::high_resolution_clock::now().time_since_epoch() - m_sTimeAtStartUp.time_since_epoch()).count();
	}

	float Timer::Now()
	{
		auto interval = std::chrono::high_resolution_clock::now() - m_sTimeAtStartUp;
		auto duration = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(interval);
		return duration.count() / 1000.0f;
	}

	unsigned int Timer::GetCurrentFPS()
	{
		return m_sCurrentFPS;
	}

	unsigned int Timer::GetAverageFPS()
	{
		return m_sAverageFPS;
	}

	float Timer::GetFrameDeltaTime()
	{
		return m_sFrameDeltaTime;
	}

	uint64_t Timer::GetCurrentFrame()
	{
		return m_frameCount;
	}
}