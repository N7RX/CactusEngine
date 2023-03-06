#pragma once
#include "SafeBasicTypes.h"

namespace Engine
{
	void SafeCounter::GetCountValue(uint32_t& val) const
	{
		val = m_countImpl;
	}

	void SafeCounter::Tick()
	{
		m_countImpl++;
	}

	bool SafeCounter::Decrease()
	{
		if (m_countImpl == 0)
		{
			return false;
		}
		else
		{
			m_countImpl--;
			return true;
		}
	}

	bool SafeCounter::Equals(uint32_t val) const
	{
		return m_countImpl == val;
	}

	void SafeCounter::Reset()
	{
		m_countImpl = 0;
	}

	void ThreadSemaphore::Signal()
	{
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_signaled = true;
		}

		m_cv.notify_all();
	}

	void ThreadSemaphore::Wait()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this]() { return m_signaled; });
		m_signaled = false;
	}
}