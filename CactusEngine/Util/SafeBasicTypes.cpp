#pragma once
#include "SafeBasicTypes.h"

void SafeBool::GetValue(bool& val) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	val = m_boolImpl;
}

void SafeBool::AssignValue(bool val)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_boolImpl = val;
}