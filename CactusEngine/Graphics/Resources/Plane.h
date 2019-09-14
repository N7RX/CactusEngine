#pragma once
#include "Mesh.h"

namespace Engine
{
	class Plane : public Mesh
	{
	public:
		Plane(uint32_t dimLength, uint32_t dimWidth); // The dimension is grid count (1, 2, 3, ...)
		~Plane() = default;
	};
}