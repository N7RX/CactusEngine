#pragma once

class NoCopy
{
public:
	NoCopy() = default;
	virtual ~NoCopy() = default;

private:
	NoCopy(const NoCopy& copyFrom) {};
};