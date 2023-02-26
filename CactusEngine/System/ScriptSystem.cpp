#include "ScriptSystem.h"
#include "AllComponents.h"

namespace Engine
{
	ScriptSystem::ScriptSystem(ECSWorld* pWorld)
		: m_pECSWorld(pWorld)
	{

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
		auto pEntityList = m_pECSWorld->GetEntityList();
		for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
		{
			auto pScriptComp = std::static_pointer_cast<ScriptComponent>(itr->second->GetComponent(EComponentType::Script));
			if (pScriptComp)
			{
				auto pScript = pScriptComp->GetScript();
				if (pScript)
				{
					if (pScript->ShouldCallStart()) // TODO: find a better solution  (e.g. start list)
					{
						pScript->Start();
					}

					pScript->Update();
				}
			}
		}
	}

	void ScriptSystem::FrameEnd()
	{

	}
}