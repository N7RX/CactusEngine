#pragma once
#include "Configuration.h"
#include "LogUtility.h"
#include "MemoryAllocator.h"

#include <stdexcept>
#include <vector>

namespace Engine
{
	class BaseApplication;
	class Global
	{
	public:
		Global();
		~Global() = default;

		void SetApplication(BaseApplication* pApp);
		BaseApplication* GetCurrentApplication() const;

		template<typename T>
		inline void CreateConfiguration(EConfigurationType configType)
		{
			DEBUG_ASSERT_CE((uint32_t)configType < m_configurations.size());
			CE_NEW(m_configurations[(uint32_t)configType], T);
		}

		template<typename T>
		inline T* GetConfiguration(EConfigurationType configType) const
		{
			DEBUG_ASSERT_CE((uint32_t)configType < m_configurations.size());
			return (T*)(m_configurations[(uint32_t)configType]);
		}

		bool QueryGlobalState(EGlobalStateQueryType type) const;
		void MarkGlobalState(EGlobalStateQueryType type, bool val); // Alert: this does not "change" global state, that's why it's "Mark.."

		void* GetWindowHandle() const;

	private:
		BaseApplication* m_pCurrentApp;
		std::vector<BaseConfiguration*> m_configurations;

		std::vector<bool> m_globalStates;
	};

	extern Global* gpGlobal;
}