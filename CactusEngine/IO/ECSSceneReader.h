#pragma once
#include "ECSWorld.h"

namespace Engine
{
	extern bool ReadECSWorldFromJson(ECSWorld* pWorld, const char* fileAddress);
}