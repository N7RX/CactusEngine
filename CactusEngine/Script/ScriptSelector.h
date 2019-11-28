#pragma once
#include <iostream>
#include "CameraScript.h"

namespace SampleScript
{
	// TODO: find a better solution
	std::shared_ptr<Engine::IScript> GenerateScriptByID(EScriptID id, const std::shared_ptr<Engine::IEntity> pEntity)
	{
		switch (id)
		{
		case eScript_Camera:
			return std::make_shared<CameraScript>(pEntity);
		default:
			std::cout << "ScriptSelector: Unhandled script type: " << id << std::endl;
			break;
		}

		return nullptr;
	}
}