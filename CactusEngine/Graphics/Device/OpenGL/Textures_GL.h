#pragma once
#include "GraphicsResources.h"
#include "GraphicsHardwareInterface_GL.h"

namespace Engine
{
	class Texture2D_GL : public Texture2D
	{
	public:
		Texture2D_GL();
		~Texture2D_GL();

		GLuint GetGLTextureID() const;
		void SetGLTextureID(GLuint id);

		void MarkTextureSize(uint32_t width, uint32_t height);

		// Not used in OpenGL device
		bool HasSampler() const override { return false; }
		void SetSampler(const TextureSampler* pSampler) override {}
		TextureSampler* GetSampler() const override { return nullptr; }

	private:
		GLuint m_glTextureID;
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