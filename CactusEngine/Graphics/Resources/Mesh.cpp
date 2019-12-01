#include "Mesh.h"

using namespace Engine;

Mesh::Mesh(const std::shared_ptr<DrawingDevice> pDevice)
	: m_pDevice(pDevice), m_type(eBuiltInMesh_External), m_planeDimension(0, 0)
{
}

std::shared_ptr<VertexBuffer> Mesh::GetVertexBuffer() const
{
	return m_pVertexBuffer;
}

const std::vector<SubMesh>* Mesh::GetSubMeshes() const
{
	return &m_subMeshes;
}

unsigned int Mesh::GetSubmeshCount() const
{
	return m_subMeshes.size();
}

const char* Mesh::GetFilePath() const
{
	return m_filePath.c_str();
}

EBuiltInMeshType Mesh::GetMeshType() const
{
	return m_type;
}

Vector2 Mesh::GetPlaneDimenstion() const
{
	return m_planeDimension;
}

void Mesh::CreateVertexBufferFromVertices(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, std::vector<float>& tangents, std::vector<float>& bitangents, std::vector<int>& indices)
{
	if (!m_pDevice)
	{
		throw std::runtime_error("Device is not assigned.");
		return;
	}

	VertexBufferCreateInfo createInfo = {};

	createInfo.pIndexData = indices.data();
	createInfo.indexDataCount = static_cast<uint32_t>(indices.size());
	createInfo.pPositionData = positions.data();
	createInfo.positionDataCount = static_cast<uint32_t>(positions.size());
	createInfo.pNormalData = normals.data();
	createInfo.normalDataCount = static_cast<uint32_t>(normals.size());
	createInfo.pTexcoordData = texcoords.data();
	createInfo.texcoordDataCount = static_cast<uint32_t>(texcoords.size());
	createInfo.pTangentData = tangents.data();
	createInfo.tangentDataCount = static_cast<uint32_t>(tangents.size());
	createInfo.pBitangentData = bitangents.data();
	createInfo.bitangentDataCount = static_cast<uint32_t>(bitangents.size());

	m_pDevice->CreateVertexBuffer(createInfo, m_pVertexBuffer);
}
