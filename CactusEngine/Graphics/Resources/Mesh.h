#pragma once
#include "DrawingResources.h"
#include "DrawingDevice.h"
#include <memory>
#include <vector>

namespace Engine
{
	class Mesh
	{
	public:
		virtual ~Mesh() = default;

		std::shared_ptr<VertexBuffer> GetVertexBuffer() const;

	protected:
		Mesh(const std::shared_ptr<DrawingDevice> pDevice);

		void CreateVertexBufferFromVertices(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texcoords, std::vector<int>& indices);

	protected:
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::shared_ptr<VertexBuffer> m_pVertexBuffer;
	};
}