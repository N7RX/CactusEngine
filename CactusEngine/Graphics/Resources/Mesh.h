#pragma once
#include "GraphicsResources.h"
#include "GraphicsDevice.h"
#include <memory>
#include <vector>

namespace Engine
{
	struct SubMesh
	{
		unsigned int m_numIndices;
		unsigned int m_baseIndex;
		unsigned int m_baseVertex;
	};

	class Mesh
	{
	public:
		virtual ~Mesh() = default;

		std::shared_ptr<VertexBuffer> GetVertexBuffer() const;
		const std::vector<SubMesh>* GetSubMeshes() const;
		unsigned int GetSubmeshCount() const;
		const char* GetFilePath() const;
		EBuiltInMeshType GetMeshType() const;
		Vector2 GetPlaneDimenstion() const;

	protected:
		Mesh(const std::shared_ptr<GraphicsDevice> pDevice);

		void CreateVertexBufferFromVertices(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, std::vector<float>& tangents, std::vector<float>& bitangents, std::vector<int>& indices);

	protected:
		std::shared_ptr<GraphicsDevice> m_pDevice;
		std::shared_ptr<VertexBuffer> m_pVertexBuffer;
		std::vector<SubMesh> m_subMeshes;

		std::string m_filePath;
		EBuiltInMeshType m_type;
		Vector2 m_planeDimension;
	};
}