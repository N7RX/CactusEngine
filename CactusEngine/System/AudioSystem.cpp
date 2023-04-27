#include "AudioSystem.h"

#include <thread>

namespace Engine
{
	void AudioSystem::Initialize()
	{

	}

	void AudioSystem::ShutDown()
	{

	}

	void AudioSystem::FrameBegin()
	{

	}

	void AudioSystem::Tick()
	{
		// Simulation for performance cost, will be removed
		//std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	void AudioSystem::FrameEnd()
	{

	}
}