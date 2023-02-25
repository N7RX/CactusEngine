#include "BaseSystem.h"

using namespace Engine;

BaseSystem::BaseSystem()
	: m_systemID(-1)
{

}

void BaseSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t BaseSystem::GetSystemID() const
{
	return m_systemID;
}