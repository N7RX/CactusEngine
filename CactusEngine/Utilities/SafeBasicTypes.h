#pragma once
#include <mutex>
#include <atomic>

namespace Engine
{
	class SafeCounter
	{
	public:
		SafeCounter() : m_countImpl(0) {};

		void GetCountValue(uint32_t& val) const;
		void Tick();
		bool Decrease();
		bool Equals(uint32_t val) const;
		void Reset();

	private:
		std::atomic<uint32_t> m_countImpl;
	};

	class ThreadSemaphore
	{
	public:
		ThreadSemaphore() : m_signaled(false) {};

		void Signal();
		void Wait();
		void Reset();
		void UnsafeReset();

	private:
		std::mutex m_mutex;
		std::condition_variable m_cv;
		bool m_signaled;
	};
}