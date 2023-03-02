#pragma once
#include "ECSWorld.h"

namespace Engine
{
	extern bool WriteECSWorldToJson(const ECSWorld* pWorld, const char* fileAddress);
}