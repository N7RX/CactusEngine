#pragma once
#include "Global.h"
#include "NoCopy.h"

namespace Engine
{
	class BaseApplication : public NoCopy
	{
	public:
		BaseApplication();
		virtual ~BaseApplication() = default;

		void SetApplicationID(uint32_t id);
		uint32_t GetApplicationID() const;

		virtual void Initialize() = 0;
		virtual void Tick() = 0;
		virtual void ShutDown() = 0;

		virtual bool ShouldQuit() const;

		virtual void* GetWindowHandle() const = 0;

	protected:
		uint32_t m_applicationID;
		bool m_shouldQuit;
	};
}