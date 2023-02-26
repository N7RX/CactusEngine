#pragma once
#include "Configuration.h"
#include "LogUtility.h"

#include <memory>
#include <stdexcept>
#include <vector>

#define GLFW_IMPLEMENTATION_CE

namespace Engine
{
	class BaseApplication;
	class Global
	{
	public:
		Global();
		~Global() = default;

		void SetApplication(const std::shared_ptr<BaseApplication> pApp);
		std::shared_ptr<BaseApplication> GetCurrentApplication() const;

		template<typename T>
		inline void CreateConfiguration(EConfigurationType configType)
		{
			DEBUG_ASSERT_CE((uint32_t)configType < m_configurations.size());
			m_configurations[(uint32_t)configType] = std::make_shared<T>();
		}

		template<typename T>
		inline std::shared_ptr<T> GetConfiguration(EConfigurationType configType) const
		{
			DEBUG_ASSERT_CE((uint32_t)configType < m_configurations.size());
			return std::dynamic_pointer_cast<T>(m_configurations[(uint32_t)configType]);
		}

		bool QueryGlobalState(EGlobalStateQueryType type) const;
		void MarkGlobalState(EGlobalStateQueryType type, bool val); // Alert: this does not "change" global state, that's why it's "Mark.."

		void* GetWindowHandle() const;

	private:
		std::shared_ptr<BaseApplication> m_pCurrentApp;
		std::vector<std::shared_ptr<BaseConfiguration>> m_configurations;

		std::vector<bool> m_globalStates;
	};

	extern Global* gpGlobal;
}