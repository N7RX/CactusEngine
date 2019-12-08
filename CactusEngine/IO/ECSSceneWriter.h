#pragma once
#include "ECSWorld.h"

namespace Engine
{
	bool WriteECSWorldToJson(const std::shared_ptr<ECSWorld> pWorld, const char* fileAddress);
}