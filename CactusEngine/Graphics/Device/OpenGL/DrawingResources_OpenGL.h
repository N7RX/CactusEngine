#pragma once
#include "DrawingResources.h"
#include "DrawingDevice_OpenGL.h"

namespace Engine
{
	class VertexBuffer_OpenGL : public VertexBuffer
	{
	public:
		VertexBuffer_OpenGL();
		~VertexBuffer_OpenGL();

	public:
		unsigned int m_vao;
		unsigned int m_vboIndices;
		unsigned int m_vboVertices;
	};

	class Texture2D_OpenGL : public Texture2D
	{
	public:
		Texture2D_OpenGL();
		~Texture2D_OpenGL();

		GLuint GetGLTextureID() const;
		void SetGLTextureID(GLuint id);

		void MarkTextureSize(uint32_t width, uint32_t height);

		// Not used in OpenGL device
		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	private:
		GLuint m_glTextureID;
	};

	class UniformBuffer_OpenGL : public UniformBuffer
	{
	public:
		~UniformBuffer_OpenGL();

		void SetGLBufferID(GLuint id);
		GLuint GetGLBufferID() const;

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;

	private:
		GLuint m_glBufferID = -1;
	};

	class FrameBuffer_OpenGL : public FrameBuffer
	{
	public:
		~FrameBuffer_OpenGL();

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

	class VertexShader_OpenGL : public VertexShader
	{
	public:
		VertexShader_OpenGL(const char* filePath);
		~VertexShader_OpenGL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class FragmentShader_OpenGL : public FragmentShader
	{
	public:
		FragmentShader_OpenGL(const char* filePath);
		~FragmentShader_OpenGL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class ShaderProgram_OpenGL : public ShaderProgram
	{
	public:
		ShaderProgram_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader);

		GLuint GetGLProgramID() const;

		unsigned int GetParamBinding(const char* paramName) const override;
		void Reset() override;

		void UpdateParameterValue(unsigned int binding, EDescriptorType type, const std::shared_ptr<RawResource> pRes);

	private:
		void ReflectParamLocations();

	private:
		GLuint m_glProgramID;
		std::unordered_map<const char*, unsigned int> m_paramBindings;
	};
}