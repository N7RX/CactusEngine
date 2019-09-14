#include "ObjMesh.h"
#include "Global.h"
#include "GraphicsApplication.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "ObjLoader/tiny_obj_loader.h"

using namespace Engine;

ObjMesh::ObjMesh(const char* filePath)
	: Mesh(std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice())
{
	LoadMeshFromFile(filePath);
}

void ObjMesh::LoadMeshFromFile(const char* filePath)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, filePath))
	{
		throw std::runtime_error(warning + "\n" + error);
	}

	// Alert: we do not handle material-id at this moment
	// Alert: we do not handle sub-shape at this moment
	// Alert: normals and texcoords are not interpolated to vertices at this moment

	std::vector<float> normals(attributes.vertices.size(), 0);
	size_t fillIndex = 0;
	for (size_t i = 0; i < attributes.vertices.size(); ++i)
	{
		normals[i] = attributes.normals[fillIndex++]; // Alert: this is wrong but a temp fix
		if (fillIndex >= attributes.normals.size())
		{
			fillIndex = 0;
		}
	}

	std::vector<float> texcoords(attributes.vertices.size() * 2 / 3, 0);
	fillIndex = 0;
	for (size_t i = 0; i < (attributes.vertices.size() * 2 / 3); ++i)
	{
		texcoords[i] = attributes.texcoords[fillIndex++];
		if (fillIndex >= attributes.texcoords.size())
		{
			fillIndex = 0;
		}
	}

	std::vector<int> vertexIndices;
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			vertexIndices.emplace_back(index.vertex_index);
		}
	}

	CreateVertexBufferFromVertices(attributes.vertices, normals, texcoords, vertexIndices);
}