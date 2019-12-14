#pragma once
#include "Mesh.h"

namespace Engine
{
	class Plane : public Mesh
	{
	public:
		Plane(uint64_t dimLength, uint64_t dimWidth); // The dimension is grid count (1, 2, 3, ...)
		~Plane() = default;
	};
}