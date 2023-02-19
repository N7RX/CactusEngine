#pragma once
#include <queue>
#include <mutex>

template<typename T>
class SafeQueue
{
public:
	SafeQueue() = default;
	SafeQueue(const SafeQueue& copyFrom);
	~SafeQueue() = default;

	void Push(T val);
	void Pop();
	bool TryPop(T& val);
	bool TryPopAll(std::queue<T>& allVals);
	T Front() const;
	void Clear();
	bool Empty() const;
	size_t Size() const;

private:
	std::queue<T> m_queueImpl;
	mutable std::mutex m_mutex;
};


template<typename T>
SafeQueue<T>::SafeQueue(const SafeQueue& copyFrom)
{
	std::lock_guard<std::mutex> lock(copyFrom.m_mutex);
	m_queueImpl = copyFrom.m_queueImpl;
}

template<typename T>
void SafeQueue<T>::Push(T val)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_queueImpl.push(std::move(val));
}

template<typename T>
void SafeQueue<T>::Pop()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_queueImpl.empty())
	{
		m_queueImpl.pop();
	}
}

template<typename T>
bool SafeQueue<T>::TryPop(T& val)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_queueImpl.empty())
	{
		return false;
	}

	val = m_queueImpl.front();
	m_queueImpl.pop();

	return true;
}

template<typename T>
bool SafeQueue<T>::TryPopAll(std::queue<T>& allVals)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_queueImpl.empty())
	{
		return false;
	}

	while (!m_queueImpl.empty())
	{
		T copy = m_queueImpl.front();
		allVals.push(copy);
		m_queueImpl.pop();
	}

	return true;
}

template<typename T>
T SafeQueue<T>::Front() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_queueImpl.front();
}

template<typename T>
void SafeQueue<T>::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_queueImpl.empty())
	{
		std::queue<T> emptyQueue;
		m_queueImpl.swap(emptyQueue);
	}
}

template<typename T>
bool SafeQueue<T>::Empty() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_queueImpl.empty();
}

template<typename T>
size_t SafeQueue<T>::Size() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_queueImpl.size();
}
