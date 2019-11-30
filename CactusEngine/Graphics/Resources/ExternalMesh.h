#pragma once
#include "Mesh.h"

namespace Engine
{
	class ExternalMesh : public Mesh
	{
	public:
		ExternalMesh(const char* filePath);
		~ExternalMesh() = default;

	private:
		void LoadMeshFromFile(const char* filePath);
	};
}