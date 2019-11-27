#pragma once
#include "Mesh.h"

namespace Engine
{
	class ObjMesh : public Mesh
	{
	public:
		ObjMesh(const char* filePath);
		~ObjMesh() = default;

	private:
		void LoadMeshFromFile(const char* filePath);
	};
}