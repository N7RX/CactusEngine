#include "Global.h"
#include "BaseApplication.h"

using namespace Engine;

namespace Engine
{
	Global* gpGlobal;
}

Global::Global()
{
	m_configurations.resize(ECONFIGURATION_COUNT, nullptr);
	m_globalStates.resize(EGLOBALSTATEQUERYTYPE_COUNT, false);
}

void Global::SetApplication(const std::shared_ptr<BaseApplication> pApp)
{
	m_pCurrentApp = pApp;
}

std::shared_ptr<BaseApplication> Global::GetCurrentApplication() const
{
	return m_pCurrentApp;
}

bool Global::QueryGlobalState(EGlobalStateQueryType type) const
{
	assert(type < m_globalStates.size());
	return m_globalStates[type];
}

void Global::MarkGlobalState(EGlobalStateQueryType type, bool val)
{
	assert(type < m_globalStates.size());
	m_globalStates[type] = val;
}

void* Global::GetWindowHandle() const
{
	return m_pCurrentApp->GetWindowHandle();
}