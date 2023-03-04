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

		// Smaller number means higher priority, 0 is the highest priority; Systems with same priority have no guarantee on execution sequence
		void SetSystemPriority(uint32_t priority);
		uint32_t GetSystemPriority() const;

		virtual void Initialize() = 0;
		virtual void ShutDown() = 0;

		virtual void FrameBegin() = 0;
		virtual void Tick() = 0;
		virtual void FrameEnd() = 0;

	protected:
		uint32_t m_systemID;
		uint32_t m_systemPriority;
	};
}