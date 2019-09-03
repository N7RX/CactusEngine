#pragma once
#include "IApplication.h"
#include "Global.h"

namespace Engine
{
	class BaseApplication : public IApplication
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

	protected:
		uint32_t m_applicationID;
		bool m_shouldQuit;
	};
}