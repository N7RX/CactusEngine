#pragma once
#include "Mesh.h"

namespace Engine
{
	struct GLCTriangle
	{
		GLCTriangle(Vector3 vert1, Vector3 vert2, Vector3 vert3)
			: v1(vert1), v2(vert2), v3(vert3)
		{}

		Vector3 v1, v2, v3;
	};

	class GLCMesh : public Mesh
	{
	public:
		GLCMesh(const char* filePath);
		~GLCMesh() = default;

		std::vector<GLCTriangle> GetTriangles() const;

	private:
		void LoadMeshFromFile(const char* filePath);

	private:
		std::vector<GLCTriangle> m_triangles;
	};
}