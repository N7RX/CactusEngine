#include "GLCMesh.h"
#include "ObjLoader/tiny_obj_loader.h"
#include "Global.h"

using namespace Engine;

GLCMesh::GLCMesh(const char* filePath)
	: Mesh(nullptr)
{
	LoadMeshFromFile(filePath);
}

std::vector<GLCTriangle> GLCMesh::GetTriangles() const
{
	return m_triangles;
}

void GLCMesh::LoadMeshFromFile(const char* filePath)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, filePath))
	{
		throw std::runtime_error(warning + "\n" + error);
	}

	std::vector<Vector3> vertices;
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			vertices.emplace_back(attributes.vertices[index.vertex_index * 3], 
				attributes.vertices[index.vertex_index * 3 + 1],
				attributes.vertices[index.vertex_index * 3 + 2]);
		}
	}

	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		m_triangles.emplace_back(vertices[i], vertices[i + 1], vertices[i + 2]);
	}
}