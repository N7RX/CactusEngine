#include "DrawingResources_OpenGL.h"
#include "DrawingUtil_OpenGL.h"
#include "BuiltInShaderType.h"

using namespace Engine;

VertexBuffer_OpenGL::VertexBuffer_OpenGL()
	: m_vao(-1), m_vboIndices(-1), m_vboPositions(-1), m_vboNormals(-1), m_vboTexcoords(-1)
{
}

VertexBuffer_OpenGL::~VertexBuffer_OpenGL()
{
	glDeleteBuffers(1, &m_vboIndices);
	glDeleteBuffers(1, &m_vboPositions);
	glDeleteBuffers(1, &m_vboNormals);
	glDeleteBuffers(1, &m_vboTexcoords);
	glDeleteVertexArrays(1, &m_vao);
}

GLuint Texture2D_OpenGL::GetGLTextureID() const
{
	return m_glTextureID;
}

uint32_t Texture2D_OpenGL::GetTextureID() const
{
	return static_cast<uint32_t>(m_glTextureID);
}

void Texture2D_OpenGL::SetGLTextureID(GLuint id)
{
	m_glTextureID = id;
}

void Texture2D_OpenGL::MarkTextureSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

VertexShader_OpenGL::VertexShader_OpenGL(const char* filePath)
{
	GLchar* source = ReadSourceFileAsChar(filePath);
	if (source == NULL)
	{
		throw std::runtime_error("Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("Failed to compile shader.");
	}
}

GLuint VertexShader_OpenGL::GetGLShaderID() const
{
	return m_glShaderID;
}

FragmentShader_OpenGL::FragmentShader_OpenGL(const char* filePath)
{
	GLchar* source = ReadSourceFileAsChar(filePath);
	if (source == NULL)
	{
		throw std::runtime_error("Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("Failed to compile shader.");
	}
}

GLuint FragmentShader_OpenGL::GetGLShaderID() const
{
	return m_glShaderID;
}

ShaderProgram_OpenGL::ShaderProgram_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader)
	: ShaderProgram(eShader_Vertex | eShader_Fragment), m_activeTextureUnit(0)
{
	m_pDevice = pDevice;
	m_glProgramID = glCreateProgram();

	glAttachShader(m_glProgramID, pVertexShader->GetGLShaderID());
	glAttachShader(m_glProgramID, pFragmentShader->GetGLShaderID());

	glLinkProgram(m_glProgramID);

	GLint linked;
	glGetProgramiv(m_glProgramID, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		PrintProgramLinkError_GL(m_glProgramID);
		throw std::runtime_error("Shader program failed to link.");
	}

	ReflectParamLocations();
}

GLuint ShaderProgram_OpenGL::GetGLProgramID() const
{
	return m_glProgramID;
}

void ShaderProgram_OpenGL::UpdateParameterValue(GLuint location, EGLShaderParamType type, const void* value)
{
	switch (type)
	{
	case eShaderParam_Int1:
		glUniform1i(location, *(int*)value);
		break;
	case eShaderParam_Float1:
		glUniform1f(location, *(float*)value);
		break;
	case eShaderParam_Vec2:
		glUniform2fv(location, 1, (float*)value);
		break;
	case eShaderParam_Vec3:
		glUniform3fv(location, 1, (float*)value);
		break;
	case eShaderParam_Vec4:
		glUniform4fv(location, 1, (float*)value);
		break;
	case eShaderParam_Mat2:
		glUniformMatrix2fv(location, 1, false, (float*)value);
		break;
	case eShaderParam_Mat3:
		glUniformMatrix3fv(location, 1, false, (float*)value);
		break;
	case eShaderParam_Mat4:
		glUniformMatrix4fv(location, 1, false, (float*)value);
		break;
	case eShaderParam_Texture2D:
		glActiveTexture(GL_TEXTURE0 + m_activeTextureUnit);
		glBindTexture(GL_TEXTURE_2D, *(unsigned int*)value);
		glUniform1i(location, m_activeTextureUnit);
		m_activeTextureUnit += 1;
		break;
	default:
		throw std::runtime_error("Unsupported shader parameter type.");
		break;
	}
}

void ShaderProgram_OpenGL::Reset()
{
	m_activeTextureUnit = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void ShaderProgram_OpenGL::ReflectParamLocations()
{
	m_paramLocations.emplace(ShaderParamNames::PVM_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::PVM_MATRIX));
	m_paramLocations.emplace(ShaderParamNames::ALBEDO_COLOR, glGetUniformLocation(m_glProgramID, ShaderParamNames::ALBEDO_COLOR));
	m_paramLocations.emplace(ShaderParamNames::ALBEDO_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::ALBEDO_TEXTURE));
}