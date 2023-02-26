#pragma once

namespace Engine
{
	class NoCopy
	{
	public:
		NoCopy() = default;
		virtual ~NoCopy() = default;

		NoCopy(const NoCopy& copyFrom) = delete;
	};
}