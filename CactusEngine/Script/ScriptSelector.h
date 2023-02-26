#pragma once
#include "SampleScript/CameraScript.h"
#include "SampleScript/CubeScript.h"
#include "SampleScript/BunnyScript.h"
#include "SampleScript/LightScript.h"
#include "LogUtility.h"

namespace SampleScript
{
	// TODO: find a better solution
	inline std::shared_ptr<Engine::BaseScript> GenerateScriptByID(EScriptID id, const std::shared_ptr<Engine::BaseEntity> pEntity)
	{
		switch (id)
		{
		case EScriptID::Camera:
			return std::make_shared<CameraScript>(pEntity);
		case EScriptID::Cube:
			return std::make_shared<CubeScript>(pEntity);
		case EScriptID::Bunny:
			return std::make_shared<BunnyScript>(pEntity);
		case EScriptID::Light:
			return std::make_shared<LightScript>(pEntity);
		default:
			Engine::LOG_WARNING("ScriptSelector: Unhandled script type: " + std::to_string((uint32_t)id));
			break;
		}

		return nullptr;
	}
}