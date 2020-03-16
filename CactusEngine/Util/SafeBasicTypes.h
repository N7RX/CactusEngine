#pragma once
#include <mutex>
#include <atomic>

class SafeCounter
{
public:
	SafeCounter() : m_countImpl(0) {};

	void GetCountValue(unsigned int& val) const;
	void Tick();
	bool Decrease();
	bool Equals(unsigned int val) const;
	void Reset();

private:
	std::atomic<unsigned int> m_countImpl;
};

class ThreadSemaphore
{
public:
	ThreadSemaphore() : m_signaled(false) {};

	void Signal();
	void Wait();

private:
	std::mutex m_mutex;
	std::condition_variable m_cv;
	bool m_signaled;
};