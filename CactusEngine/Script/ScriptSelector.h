#pragma once
#include "SampleScript/CameraScript.h"
#include "SampleScript/CubeScript.h"
#include "SampleScript/BunnyScript.h"
#include "SampleScript/LightScript.h"

#include <iostream>

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
			std::cout << "ScriptSelector: Unhandled script type: " << (uint32_t)id << std::endl;
			break;
		}

		return nullptr;
	}
}