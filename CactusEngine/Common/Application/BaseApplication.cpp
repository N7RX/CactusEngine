#include "BaseApplication.h"

using namespace Engine;

BaseApplication::BaseApplication()
	: m_shouldQuit(false)
{
}

void BaseApplication::SetApplicationID(uint32_t id)
{
	m_applicationID = id;
}

uint32_t BaseApplication::GetApplicationID() const
{
	return m_applicationID;
}

bool BaseApplication::ShouldQuit() const
{
	return m_shouldQuit;
}