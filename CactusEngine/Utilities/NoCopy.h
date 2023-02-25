#pragma once

class NoCopy
{
public:
	NoCopy() = default;
	virtual ~NoCopy() = default;

	NoCopy(const NoCopy& copyFrom) = delete;
};