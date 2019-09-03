#pragma once
#include "Mesh.h"

namespace Engine
{
	struct SubMesh
	{
		std::string m_name;
		unsigned int m_numIndices;
		unsigned int m_baseIndex;
		unsigned int m_baseVertex;
	};

	class ObjMesh : public Mesh
	{
	public:
		ObjMesh(const char* filePath);
		~ObjMesh() = default;

	private:
		void LoadMeshFromFile(const char* filePath);

	private:
		std::vector<SubMesh> m_subShapes;
	};
}