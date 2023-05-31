#pragma once
#include "GraphicsResources.h"

#include <vector>

namespace Engine
{
	struct SubMesh
	{
		uint32_t m_numIndices;
		uint32_t m_baseIndex;
		uint32_t m_baseVertex;
	};

	class Mesh
	{
	public:
		virtual ~Mesh() = default;

		VertexBuffer* GetVertexBuffer() const;
		const std::vector<SubMesh>* GetSubMeshes() const;
		uint32_t GetSubmeshCount() const;
		const char* GetFilePath() const;
		EBuiltInMeshType GetMeshType() const;
		Vector2 GetPlaneDimenstion() const;

	protected:
		Mesh(GraphicsDevice* pDevice);

		void CreateVertexBufferFromVertices(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, std::vector<float>& tangents, std::vector<int>& indices);

	protected:
		GraphicsDevice* m_pDevice;
		VertexBuffer* m_pVertexBuffer;
		std::vector<SubMesh> m_subMeshes;

		std::string m_filePath;
		EBuiltInMeshType m_type;
		Vector2 m_planeDimension;
	};
}