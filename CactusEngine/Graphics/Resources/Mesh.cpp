#include "Mesh.h"

using namespace Engine;

Mesh::Mesh(const std::shared_ptr<DrawingDevice> pDevice)
	: m_pDevice(pDevice)
{
}

std::shared_ptr<VertexBuffer> Mesh::GetVertexBuffer() const
{
	return m_pVertexBuffer;
}

void Mesh::CreateVertexBufferFromVertices(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, std::vector<int>& indices)
{
	VertexBufferCreateInfo createInfo = {};

	createInfo.pIndexData = indices.data();
	createInfo.indexDataCount = static_cast<uint32_t>(indices.size());
	createInfo.pPositionData = positions.data();
	createInfo.positionDatacount = static_cast<uint32_t>(positions.size());
	createInfo.pNormalData = normals.data();
	createInfo.normalDatacount = static_cast<uint32_t>(normals.size());
	createInfo.pTexcoordData = texcoords.data();
	createInfo.texcoordDatacount = static_cast<uint32_t>(texcoords.size());

	m_pDevice->CreateVertexBuffer(createInfo, m_pVertexBuffer);
}
