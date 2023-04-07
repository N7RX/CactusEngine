#pragma once
#include "GraphicsResources.h"
#include "GraphicsHardwareInterface_GL.h"

namespace Engine
{
	// Just a structure for sampling config look up
	class Sampler_GL : public TextureSampler
	{
	public:
		Sampler_GL(const TextureSamplerCreateInfo& createInfo);

	private:
		bool  m_enableAnisotropy;
		float m_anisotropyLevel;
		GLenum m_magFilter;
		GLenum m_minFilter;
		GLenum m_mipmapMode;
		float m_minLod;
		float m_maxLod;
		float m_baseLod;

		friend class Texture2D_GL;
	};

	class Texture2D_GL : public Texture2D
	{
	public:
		Texture2D_GL();
		~Texture2D_GL();

		GLuint GetGLTextureID() const;
		void SetGLTextureID(GLuint id);

		void MarkTextureSize(uint32_t width, uint32_t height);

		bool HasSampler() const override;
		void SetSampler(const TextureSampler* pSampler) override;
		TextureSampler* GetSampler() const override;

		void MarkHasMipmap();
		bool HasMipmap() const;

	private:
		GLuint m_glTextureID;
		bool m_hasMipmap;

		Sampler_GL* m_pSampler;
	};

	class FrameBuffer_GL : public FrameBuffer
	{
	public:
		FrameBuffer_GL();
		~FrameBuffer_GL();

		GLuint GetGLFrameBufferID() const;
		uint32_t GetFrameBufferID() const override;

		void SetGLFrameBufferID(GLuint id);
		void MarkFrameBufferSize(uint32_t width, uint32_t height);

		void AddColorAttachment(GLenum attachment);
		size_t GetColorAttachmentCount() const;
		GLenum GetColorAttachment(uint32_t index) const;
		const GLenum* GetColorAttachments() const;

		bool IsRenderToBackbuffer() const;
		void MarkRenderToBackbuffer();

	private:
		GLuint m_glFrameBufferID;
		std::vector<GLenum> m_bufferAttachments;

		bool m_isRenderToBackbuffer;
	};
}