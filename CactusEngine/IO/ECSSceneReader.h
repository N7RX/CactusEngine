#pragma once
#include "ECSWorld.h"

namespace Engine
{
	bool ReadECSWorldFromJson(std::shared_ptr<ECSWorld> pWorld, const char* fileAddress);
}