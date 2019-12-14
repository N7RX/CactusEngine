#pragma once
#include <mutex>

class SafeBool
{
public:
	SafeBool() : m_boolImpl(false) {};

	void GetValue(bool& val) const;
	void AssignValue(bool val);

private:
	bool m_boolImpl;
	mutable std::mutex m_mutex;
};