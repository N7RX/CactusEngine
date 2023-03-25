#include "Textures_GL.h"
#include "GHIUtilities_GL.h"
#include "LogUtility.h"

namespace Engine
{
	Texture2D_GL::Texture2D_GL()
		: Texture2D(ETexture2DSource::RawDeviceTexture),
		m_glTextureID(-1)
	{

	}

	Texture2D_GL::~Texture2D_GL()
	{
		glDeleteTextures(1, &m_glTextureID);
	}

	GLuint Texture2D_GL::GetGLTextureID() const
	{
		return m_glTextureID;
	}

	void Texture2D_GL::SetGLTextureID(GLuint id)
	{
		m_glTextureID = id;
	}

	void Texture2D_GL::MarkTextureSize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
	}

	FrameBuffer_GL::~FrameBuffer_GL()
	{
		glDeleteFramebuffers(1, &m_glFrameBufferID);
	}

	GLuint FrameBuffer_GL::GetGLFrameBufferID() const
	{
		return m_glFrameBufferID;
	}

	uint32_t FrameBuffer_GL::GetFrameBufferID() const
	{
		return static_cast<uint32_t>(m_glFrameBufferID);
	}

	void FrameBuffer_GL::SetGLFrameBufferID(GLuint id)
	{
		m_glFrameBufferID = id;
	}

	void FrameBuffer_GL::MarkFrameBufferSize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
	}

	void FrameBuffer_GL::AddColorAttachment(GLenum attachment)
	{
		m_bufferAttachments.emplace_back(attachment);
	}

	size_t FrameBuffer_GL::GetColorAttachmentCount() const
	{
		return m_bufferAttachments.size();
	}

	GLenum FrameBuffer_GL::GetColorAttachment(uint32_t index) const
	{
		DEBUG_ASSERT_CE(index < m_bufferAttachments.size());
		return m_bufferAttachments[index];
	}

	const GLenum* FrameBuffer_GL::GetColorAttachments() const
	{
		return m_bufferAttachments.data();
	}
}