#pragma once
#include <iostream>
#include "CameraScript.h"
#include "CubeScript.h"
#include "BunnyScript.h"

namespace SampleScript
{
	// TODO: find a better solution
	std::shared_ptr<Engine::IScript> GenerateScriptByID(EScriptID id, const std::shared_ptr<Engine::IEntity> pEntity)
	{
		switch (id)
		{
		case eScript_Camera:
			return std::make_shared<CameraScript>(pEntity);
		case eScript_Cube:
			return std::make_shared<CubeScript>(pEntity);
		case eScript_Bunny:
			return std::make_shared<BunnyScript>(pEntity);
		default:
			std::cout << "ScriptSelector: Unhandled script type: " << id << std::endl;
			break;
		}

		return nullptr;
	}
}