#include "Global.h"
#include "BaseApplication.h"

namespace Engine
{
	Global* gpGlobal;

	Global::Global()
	{
		m_configurations.resize((uint32_t)EConfigurationType::COUNT, nullptr);
		m_globalStates.resize((uint32_t)EGlobalStateQueryType::COUNT, false);
	}

	void Global::SetApplication(BaseApplication* pApp)
	{
		m_pCurrentApp = pApp;
	}

	BaseApplication* Global::GetCurrentApplication() const
	{
		return m_pCurrentApp;
	}

	bool Global::QueryGlobalState(EGlobalStateQueryType type) const
	{
		DEBUG_ASSERT_CE((uint32_t)type < m_globalStates.size());
		return m_globalStates[(uint32_t)type];
	}

	void Global::MarkGlobalState(EGlobalStateQueryType type, bool val)
	{
		DEBUG_ASSERT_CE((uint32_t)type < m_globalStates.size());
		m_globalStates[(uint32_t)type] = val;
	}

	void* Global::GetWindowHandle() const
	{
		return m_pCurrentApp->GetWindowHandle();
	}
}