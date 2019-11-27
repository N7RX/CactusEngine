#include "ScriptSystem.h"
#include "AllComponents.h"

using namespace Engine;

ScriptSystem::ScriptSystem(ECSWorld* pWorld)
	: m_pECSWorld(pWorld)
{

}

void ScriptSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t ScriptSystem::GetSystemID() const
{
	return m_systemID;
}

void ScriptSystem::Initialize()
{

}

void ScriptSystem::ShutDown()
{

}

void ScriptSystem::FrameBegin()
{

}

void ScriptSystem::Tick()
{
	const EntityList* pEntityList = m_pECSWorld->GetEntityList();
	for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
	{
		auto pScriptComp = std::static_pointer_cast<ScriptComponent>(itr->second->GetComponent(eCompType_Script));
		if (pScriptComp)
		{
			auto pScript = pScriptComp->GetScript();
			if (pScript)
			{
				pScript->Update();
			}
		}
	}
}

void ScriptSystem::FrameEnd()
{

}