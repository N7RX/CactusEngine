#pragma once
#include <mutex>

class SafeBool
{
public:
	SafeBool() : m_boolImpl(false) {};

	void GetValue(bool& val) const;
	bool Get() const;
	void AssignValue(bool val);

private:
	bool m_boolImpl;
	mutable std::mutex m_mutex;
};

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
	unsigned int m_countImpl;
	mutable std::mutex m_mutex;
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