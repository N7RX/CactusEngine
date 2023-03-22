#include "Buffers_GL.h"
#include "GHIUtilities_GL.h"
#include "LogUtility.h"

namespace Engine
{
	VertexBuffer_GL::VertexBuffer_GL()
		: m_vao(-1),
		m_vboIndices(-1),
		m_vboVertices(-1)
	{
	}

	VertexBuffer_GL::~VertexBuffer_GL()
	{
		glDeleteBuffers(1, &m_vboIndices);
		glDeleteBuffers(1, &m_vboVertices);
		glDeleteVertexArrays(1, &m_vao);
	}

	UniformBuffer_GL::~UniformBuffer_GL()
	{
		glDeleteBuffers(1, &m_glBufferID);
	}

	void UniformBuffer_GL::SetGLBufferID(GLuint id)
	{
		m_glBufferID = id;
	}

	GLuint UniformBuffer_GL::GetGLBufferID() const
	{
		return m_glBufferID;
	}

	void UniformBuffer_GL::UpdateBufferData(const void* pData, const SubUniformBuffer* pSubBuffer)
	{
		DEBUG_ASSERT_CE(m_sizeInBytes != 0);
		DEBUG_ASSERT_CE(m_glBufferID != -1);
		// SubUniformBuffer is ignored, always update whole buffer

		glBindBuffer(GL_UNIFORM_BUFFER, m_glBufferID);
		glBufferData(GL_UNIFORM_BUFFER, m_sizeInBytes, pData, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void UniformBuffer_GL::UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size)
	{
		DEBUG_ASSERT_CE(m_sizeInBytes != 0 && size != 0 && offset + size <= m_sizeInBytes);
		DEBUG_ASSERT_CE(m_glBufferID != -1);

		glBindBuffer(GL_UNIFORM_BUFFER, m_glBufferID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, pData);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	SubUniformBuffer UniformBuffer_GL::AllocateSubBuffer(uint32_t size)
	{
		// OpenGL does not use sub buffers, thus returning the entire buffer
		return SubUniformBuffer(this, 0, m_sizeInBytes);
	}
}