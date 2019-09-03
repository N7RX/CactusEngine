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
		unsigned int m_vboPositions;
		unsigned int m_vboNormals;
		unsigned int m_vboTexcoords;
	};

	class Texture2D_OpenGL : public Texture2D
	{

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
		ShaderProgram_OpenGL(const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader);

		GLuint GetGLProgramID() const;

		void UpdateParameterValue(GLuint location, EGLShaderParamType type, const void* value);

	private:
		void ReflectParamLocations();

	private:
		GLuint m_glProgramID;
	};
}