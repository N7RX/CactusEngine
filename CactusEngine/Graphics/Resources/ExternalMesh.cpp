#include "ExternalMesh.h"
#include "Global.h"
#include "GraphicsApplication.h"
#include <iostream>
// Integration with Assimp
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace Engine;

static Assimp::Importer gImporter;

ExternalMesh::ExternalMesh(const char* filePath)
	: Mesh(std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice())
{
	LoadMeshFromFile(filePath);
}

void ExternalMesh::LoadMeshFromFile(const char* filePath)
{
	// Load model with Assimp importer

	// Alert: check these import flags when models seem incorrect
	const aiScene* scene = gImporter.ReadFile(filePath, aiProcessPreset_TargetRealtime_Quality | aiProcess_PreTransformVertices | aiProcess_FlipUVs);

	if (!scene)
	{
		std::cerr << "Could not read file: " << filePath << std::endl;
		return;
	}

	size_t totalNumSubMeshes = scene->mNumMeshes;
	size_t totalNumVertices = 0;
	size_t totalNumIndices = 0;

	m_subMeshes.resize(totalNumSubMeshes);

	// Record submeshes
	for (int i = 0; i < totalNumSubMeshes; ++i)
	{
		m_subMeshes[i].m_baseIndex = (unsigned int)totalNumIndices;
		m_subMeshes[i].m_baseVertex = (unsigned int)totalNumVertices;
		m_subMeshes[i].m_numIndices = scene->mMeshes[i]->mNumFaces * 3;

		totalNumVertices += scene->mMeshes[i]->mNumVertices;
		totalNumIndices  += (size_t)scene->mMeshes[i]->mNumFaces * 3;
	}

	std::vector<int>   indices(totalNumIndices);
	std::vector<float> vertices(totalNumVertices * 3);
	std::vector<float> normals(totalNumVertices * 3);
	std::vector<float> texcoords(totalNumVertices * 2);
	std::vector<float> tangents(totalNumVertices * 3);
	std::vector<float> bitangents(totalNumVertices * 3);

	unsigned int faceIndex = 0;
	unsigned int vertexOffset = 0;
	unsigned int normalOffset = 0;
	unsigned int texcoordOffset = 0;
	unsigned int tangentOffset = 0;
	unsigned int bitangentOffset = 0;

	// Buffer data
	for (int i = 0; i < totalNumSubMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];

		// Indices
		unsigned int numFaces = mesh->mNumFaces;
		for (unsigned int j = 0; j < numFaces; ++j)
		{
			memcpy(&indices[faceIndex], mesh->mFaces[j].mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}

		// Vertices
		if (mesh->HasPositions())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&vertices[vertexOffset], mesh->mVertices, size);
			vertexOffset += 3 * mesh->mNumVertices;
		}
		
		// Normals
		if (mesh->HasNormals())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&normals[normalOffset], mesh->mNormals, size);
			normalOffset += 3 * mesh->mNumVertices;
		}

		// Texcoords
		if (mesh->HasTextureCoords(0))
		{
			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				texcoords[(uint64_t)j * 2 + texcoordOffset] = mesh->mTextureCoords[0][j].x;
				texcoords[(uint64_t)j * 2 + 1 + texcoordOffset] = mesh->mTextureCoords[0][j].y;
			}
			texcoordOffset += 2 * mesh->mNumVertices;
		}

		// Tangents
		if (mesh->HasTangentsAndBitangents())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&tangents[tangentOffset], mesh->mTangents, size);
			tangentOffset += 3 * mesh->mNumVertices;
		}

		// Bitangents
		if (mesh->HasTangentsAndBitangents())
		{
			const int size = 3 * sizeof(float) * mesh->mNumVertices;
			memcpy(&bitangents[bitangentOffset], mesh->mBitangents, size);
			bitangentOffset += 3 * mesh->mNumVertices;
		}
	}

	m_filePath.assign(filePath);
	m_type = EBuiltInMeshType::External;
	CreateVertexBufferFromVertices(vertices, normals, texcoords, tangents, bitangents, indices);
}