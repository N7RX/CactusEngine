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
		float m_minLod;
		float m_maxLod;
		// Support more properties if needed

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

	private:
		GLuint m_glTextureID;

		Sampler_GL* m_pSampler;
	};

	class FrameBuffer_GL : public FrameBuffer
	{
	public:
		~FrameBuffer_GL();

		GLuint GetGLFrameBufferID() const;
		uint32_t GetFrameBufferID() const override;

		void SetGLFrameBufferID(GLuint id);
		void MarkFrameBufferSize(uint32_t width, uint32_t height);

		void AddColorAttachment(GLenum attachment);
		size_t GetColorAttachmentCount() const;
		GLenum GetColorAttachment(uint32_t index) const;
		const GLenum* GetColorAttachments() const;

	private:
		GLuint m_glFrameBufferID = -1;
		std::vector<GLenum> m_bufferAttachments;
	};
}