#pragma once
#include "SampleScript/CameraScript.h"
#include "SampleScript/CubeScript.h"
#include "SampleScript/BunnyScript.h"
#include "SampleScript/LightScript.h"
#include "LogUtility.h"
#include "MemoryAllocator.h"

namespace SampleScript
{
	// TODO: find a better solution
	inline Engine::BaseScript* GenerateScriptByID(EScriptID id, Engine::BaseEntity* pEntity)
	{
		Engine::BaseScript* pScript = nullptr;
		switch (id)
		{
		case EScriptID::Camera:
			Engine::CE_NEW(pScript, CameraScript, pEntity);
			break;
		case EScriptID::Cube:
			Engine::CE_NEW(pScript, CubeScript, pEntity);
			break;
		case EScriptID::Bunny:
			Engine::CE_NEW(pScript, BunnyScript, pEntity);
			break;
		case EScriptID::Light:
			Engine::CE_NEW(pScript, LightScript, pEntity);
			break;
		default:
			Engine::LOG_WARNING("ScriptSelector: Unhandled script type: " + std::to_string((uint32_t)id));
			break;
		}

		return pScript;
	}
}