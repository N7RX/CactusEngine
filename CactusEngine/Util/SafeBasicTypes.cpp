#pragma once
#include "SafeBasicTypes.h"

void SafeBool::GetValue(bool& val) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	val = m_boolImpl;
}

bool SafeBool::Get() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_boolImpl;
}

void SafeBool::AssignValue(bool val)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_boolImpl = val;
}

void SafeCounter::GetCountValue(unsigned int& val) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	val = m_countImpl;
}

void SafeCounter::Tick()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_countImpl++;
}

bool SafeCounter::Decrease()
{
	std::lock_guard<std::mutex> lock(m_mutex);
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

bool SafeCounter::Equals(unsigned int val) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_countImpl == val;
}

void SafeCounter::Reset()
{
	std::lock_guard<std::mutex> lock(m_mutex);
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