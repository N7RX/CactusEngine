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

static bool HasSmoothingGroup(const tinyobj::shape_t& shape)
{
	for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); ++i)
	{
		if (shape.mesh.smoothing_group_ids[i] > 0)
		{
			return true;
		}
	}
	return false;
}

static bool CompareIndices(const tinyobj::index_t& a, const tinyobj::index_t& b)
{
	return a.vertex_index == b.vertex_index && a.normal_index == b.normal_index && a.texcoord_index == b.texcoord_index;
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

	// Alert: we only handle triangular face
	// Alert: we do not handle material-id at this moment
	// Alert: we do not handle sub-shape at this moment

	std::vector<int> vertexIndices;
	std::vector<tinyobj::index_t> smoothIndices;
	int baseIndex = 0;

	for (const auto& shape : shapes)
	{
		if (HasSmoothingGroup(shape))
		{
			unsigned int prevGroupID = shape.mesh.smoothing_group_ids[0];
			std::vector<tinyobj::index_t> groupIndices;

			for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); ++i)
			{
				if (shape.mesh.smoothing_group_ids[i] != prevGroupID)
				{
					prevGroupID = shape.mesh.smoothing_group_ids[i];
					--i;

					std::unordered_map<int, char> filterMap;
					for (size_t j = 0; j < groupIndices.size(); ++j)
					{
						if (filterMap.find(groupIndices[j].vertex_index) == filterMap.end()) // Remove duplicate face vertices from the group
						{
							filterMap.emplace(groupIndices[j].vertex_index, '0'); // '0' is a placeholder, it has no meaning
							smoothIndices.emplace_back(groupIndices[j]);

							vertexIndices.emplace_back((smoothIndices.size() - 1));
						}
						else
						{
							for (size_t k = baseIndex; k < smoothIndices.size(); ++k)
							{
								if (CompareIndices(smoothIndices[k], groupIndices[j]))
								{
									vertexIndices.emplace_back(k);
									break;
								}
							}
						}
					}

					baseIndex = smoothIndices.size();
					groupIndices.clear();
				}
				else
				{
					groupIndices.emplace_back(shape.mesh.indices[3 * i]);
					groupIndices.emplace_back(shape.mesh.indices[3 * i + 1]);
					groupIndices.emplace_back(shape.mesh.indices[3 * i + 2]);
				}

				if (i == shape.mesh.smoothing_group_ids.size() - 1) // Last face
				{
					std::unordered_map<int, char> filterMap;
					for (size_t j = 0; j < groupIndices.size(); ++j)
					{
						if (filterMap.find(groupIndices[j].vertex_index) == filterMap.end())
						{
							filterMap.emplace(groupIndices[j].vertex_index, '0');
							smoothIndices.emplace_back(groupIndices[j]);

							vertexIndices.emplace_back((smoothIndices.size() - 1));
						}
						else
						{
							for (size_t k = baseIndex; k < smoothIndices.size(); ++k)
							{
								if (CompareIndices(smoothIndices[k], groupIndices[j]))
								{
									vertexIndices.emplace_back(k);
									break;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			smoothIndices = shape.mesh.indices;

			for (size_t k = baseIndex; k < smoothIndices.size(); ++k)
			{
				vertexIndices.emplace_back(k);
			}
			baseIndex = smoothIndices.size();
		}
	}

	std::vector<float> vertices(smoothIndices.size() * 3, 0);
	std::vector<float> normals(smoothIndices.size() * 3, 0);
	std::vector<float> texcoords(smoothIndices.size() * 2, 0);

	for (size_t i = 0; i < smoothIndices.size(); ++i)
	{
		int vertexIndex = smoothIndices[i].vertex_index;

		vertices[3 * i] = attributes.vertices[3 * vertexIndex];
		vertices[3 * i + 1] = attributes.vertices[3 * vertexIndex + 1];
		vertices[3 * i + 2] = attributes.vertices[3 * vertexIndex + 2];

		int normalIndex = smoothIndices[i].normal_index;

		normals[3 * i] = attributes.normals[3 * normalIndex];
		normals[3 * i + 1] = attributes.normals[3 * normalIndex + 1];
		normals[3 * i + 2] = attributes.normals[3 * normalIndex + 2];

		int texcoordIndex = smoothIndices[i].texcoord_index;

		texcoords[2 * i] = attributes.texcoords[2 * texcoordIndex];
		texcoords[2 * i + 1] = attributes.texcoords[2 * texcoordIndex + 1];
	}

	smoothIndices.clear();

	CreateVertexBufferFromVertices(vertices, normals, texcoords, vertexIndices);
}