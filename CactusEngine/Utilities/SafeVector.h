#pragma once
#include "LogUtility.h"

#include <vector>
#include <mutex>

namespace Engine
{
	template<typename T>
	class SafeVector
	{
	public:
		SafeVector() = default;
		SafeVector(const SafeVector& copyFrom);
		~SafeVector() = default;

		void PushBack(T& val);
		T TryGet(uint32_t index);
		void Resize(size_t size, T& initVal);
		void Clear();
		bool Empty() const;
		size_t Size() const;

	private:
		std::vector<T> m_vectorImpl;
		mutable std::mutex m_mutex;
	};


	template<typename T>
	SafeVector<T>::SafeVector(const SafeVector& copyFrom)
	{
		std::lock_guard<std::mutex> lock(copyFrom.m_mutex);
		m_vectorImpl = copyFrom.m_vectorImpl;
	}

	template<typename T>
	void SafeVector<T>::PushBack(T& val)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_vectorImpl.push(std::move(val));
	}

	template<typename T>
	T SafeVector<T>::TryGet(uint32_t index)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		DEBUG_ASSERT_CE(index < m_vectorImpl.size());
		return m_vectorImpl[index];
	}

	template<typename T>
	void SafeVector<T>::Resize(size_t size, T& initVal)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_vectorImpl.resize(size, initVal);
	}

	template<typename T>
	void SafeVector<T>::Clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_vectorImpl.clear();
	}

	template<typename T>
	bool SafeVector<T>::Empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_vectorImpl.empty();
	}

	template<typename T>
	size_t SafeVector<T>::Size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_vectorImpl.size();
	}
}