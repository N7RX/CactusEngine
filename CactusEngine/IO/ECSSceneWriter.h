#pragma once
#include "ECSWorld.h"

namespace Engine
{
	extern bool WriteECSWorldToJson(const std::shared_ptr<ECSWorld> pWorld, const char* fileAddress);
}