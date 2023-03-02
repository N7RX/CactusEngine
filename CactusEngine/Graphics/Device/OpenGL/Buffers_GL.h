#pragma once
#include "GraphicsResources.h"
#include "GraphicsHardwareInterface_GL.h"

namespace Engine
{
	class VertexBuffer_GL : public VertexBuffer
	{
	public:
		VertexBuffer_GL();
		~VertexBuffer_GL();

	public:
		unsigned int m_vao;
		unsigned int m_vboIndices;
		unsigned int m_vboVertices;
	};

	class UniformBuffer_GL : public UniformBuffer
	{
	public:
		~UniformBuffer_GL();

		void SetGLBufferID(GLuint id);
		GLuint GetGLBufferID() const;

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;
		SubUniformBuffer* AllocateSubBuffer(uint32_t size) override;
		void ResetSubBufferAllocation() override;

	private:
		GLuint m_glBufferID = -1;
	};
}