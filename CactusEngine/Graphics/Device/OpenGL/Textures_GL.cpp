#include "Textures_GL.h"
#include "GHIUtilities_GL.h"
#include "LogUtility.h"

namespace Engine
{
	Sampler_GL::Sampler_GL(const TextureSamplerCreateInfo& createInfo)
		: m_enableAnisotropy(createInfo.enableAnisotropy),
		m_anisotropyLevel(createInfo.maxAnisotropy),
		m_magFilter(OpenGLTextureFilterMode(createInfo.magMode)),
		m_minFilter(OpenGLTextureFilterMode(createInfo.minMode)),
		m_mipmapMode(OpenGLTextureMipmapMode(createInfo.mipmapMode)),
		m_minLod(createInfo.minLod),
		m_maxLod(createInfo.maxLod),
		m_baseLod(createInfo.minLodBias)
	{

	}

	Texture2D_GL::Texture2D_GL()
		: Texture2D(ETexture2DSource::RawDeviceTexture),
		m_glTextureID(-1),
		m_hasMipmap(false),
		m_pSampler(nullptr)
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

	bool Texture2D_GL::HasSampler() const
	{
		return m_pSampler != nullptr;
	}

	void Texture2D_GL::SetSampler(const TextureSampler* pSampler)
	{
		if (m_pSampler != pSampler)
		{
			if (!pSampler)
			{
				m_pSampler = nullptr;
				return;
			}

			m_pSampler = (Sampler_GL*)pSampler;

			glBindTexture(GL_TEXTURE_2D, m_glTextureID);

			if (m_pSampler->m_enableAnisotropy)
			{
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_pSampler->m_anisotropyLevel);
			}

			if (m_hasMipmap)
			{
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_pSampler->m_mipmapMode);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_pSampler->m_mipmapMode);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, m_pSampler->m_baseLod);
			}
			else
			{
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_pSampler->m_minFilter);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_pSampler->m_magFilter);
			}

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, m_pSampler->m_minLod);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, m_pSampler->m_maxLod);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	TextureSampler* Texture2D_GL::GetSampler() const
	{
		return m_pSampler;
	}

	void Texture2D_GL::MarkHasMipmap()
	{
		m_hasMipmap = true;
	}

	bool Texture2D_GL::HasMipmap() const
	{
		return m_hasMipmap;
	}

	FrameBuffer_GL::FrameBuffer_GL()
		: FrameBuffer(),
		m_glFrameBufferID(-1),
		m_isRenderToBackbuffer(false)
	{

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

	bool FrameBuffer_GL::IsRenderToBackbuffer() const
	{
		return m_isRenderToBackbuffer;
	}

	void FrameBuffer_GL::MarkRenderToBackbuffer()
	{
		m_isRenderToBackbuffer = true;
	}
}