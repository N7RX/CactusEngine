#include "ObjMesh.h"
#include "Global.h"
#include "GraphicsApplication.h"
#include <iostream>
// Integration with Assimp
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace Engine;

Assimp::Importer gImporter;

ObjMesh::ObjMesh(const char* filePath)
	: Mesh(std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice())
{
	LoadMeshFromFile(filePath);
}

void ObjMesh::LoadMeshFromFile(const char* filePath)
{
	// Load model with Assimp importer

	const aiScene* scene = gImporter.ReadFile(filePath, aiProcessPreset_TargetRealtime_Quality | aiProcess_PreTransformVertices);

	if (!scene)
	{
		std::cerr << "Could not read file: " << filePath << std::endl;
		return;
	}

	// Alert: we haven't handle submeshes yet

	int totalNumSubmeshes = scene->mNumMeshes;
	int totalNumVertices = 0;
	int totalNumIndices = 0;

	for (int i = 0; i < totalNumSubmeshes; ++i)
	{
		totalNumVertices += scene->mMeshes[i]->mNumVertices;
		totalNumIndices  += scene->mMeshes[i]->mNumFaces * 3;
	}

	std::vector<int>   indices(totalNumIndices);
	std::vector<float> vertices(totalNumVertices * 3);
	std::vector<float> normals(totalNumVertices * 3);
	std::vector<float> texcoords(totalNumVertices * 2);

	unsigned int faceIndex = 0;
	unsigned int vertexOffset = 0;
	unsigned int texcoordOffset = 0;
	unsigned int normalOffset = 0;

	for (int i = 0; i < totalNumSubmeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];

		// Indices
		int meshFaces = mesh->mNumFaces;
		for (unsigned int j = 0; j < meshFaces; ++j)
		{
			memcpy(&indices[faceIndex], scene->mMeshes[i]->mFaces[j].mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}

		// Vertices
		if (mesh->HasPositions())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&vertices[vertexOffset], mesh->mVertices, size);
			vertexOffset += 3 * mesh->mNumVertices;
		}

		// Texcoords
		if (mesh->HasTextureCoords(0))
		{
			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				texcoords[j * 2 + texcoordOffset] = mesh->mTextureCoords[0][j].x;
				texcoords[j * 2 + 1 + texcoordOffset] = mesh->mTextureCoords[0][j].y;
			}
			texcoordOffset += 2 * mesh->mNumVertices;
		}
		
		// Normals
		if (mesh->HasNormals())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&normals[normalOffset], mesh->mNormals, size);
			normalOffset += 3 * mesh->mNumVertices;
		}
	}

	CreateVertexBufferFromVertices(vertices, normals, texcoords, indices);
}