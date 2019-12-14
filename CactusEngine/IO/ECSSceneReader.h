#pragma once
#include "ECSWorld.h"

namespace Engine
{
	extern bool ReadECSWorldFromJson(std::shared_ptr<ECSWorld> pWorld, const char* fileAddress);
}