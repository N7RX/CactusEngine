#pragma once
#include "SharedTypes.h"
#include "NoCopy.h"

#include <cstdint>

namespace Engine
{
	class BaseSystem : public NoCopy
	{
	public:
		BaseSystem();
		virtual ~BaseSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		virtual void Initialize() = 0;
		virtual void ShutDown() = 0;

		virtual void FrameBegin() = 0;
		virtual void Tick() = 0;
		virtual void FrameEnd() = 0;

	protected:
		uint32_t m_systemID;
	};
}