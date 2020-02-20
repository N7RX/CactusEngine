#include "DrawingResources_OpenGL.h"
#include "DrawingUtil_OpenGL.h"
#include "BuiltInShaderType.h"

using namespace Engine;

VertexBuffer_OpenGL::VertexBuffer_OpenGL()
	: m_vao(-1), m_vboIndices(-1), m_vboVertices(-1)
{
}

VertexBuffer_OpenGL::~VertexBuffer_OpenGL()
{
	glDeleteBuffers(1, &m_vboIndices);
	glDeleteBuffers(1, &m_vboVertices);
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

FrameBuffer_OpenGL::~FrameBuffer_OpenGL()
{
	glDeleteFramebuffers(1, &m_glFrameBufferID);
}

GLuint FrameBuffer_OpenGL::GetGLFrameBufferID() const
{
	return m_glFrameBufferID;
}

uint32_t FrameBuffer_OpenGL::GetFrameBufferID() const
{
	return static_cast<uint32_t>(m_glFrameBufferID);
}

void FrameBuffer_OpenGL::SetGLFrameBufferID(GLuint id)
{
	m_glFrameBufferID = id;
}

void FrameBuffer_OpenGL::MarkFrameBufferSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void FrameBuffer_OpenGL::AddColorAttachment(GLenum attachment)
{
	m_bufferAttachments.emplace_back(attachment);
}

size_t FrameBuffer_OpenGL::GetColorAttachmentCount() const
{
	return m_bufferAttachments.size();
}

GLenum FrameBuffer_OpenGL::GetColorAttachment(uint32_t index) const
{
	assert(index < m_bufferAttachments.size());

	return m_bufferAttachments[index];
}

const GLenum* FrameBuffer_OpenGL::GetColorAttachments() const
{
	return m_bufferAttachments.data();
}

VertexShader_OpenGL::VertexShader_OpenGL(const char* filePath)
{
	GLchar* source = ReadSourceFileAsChar(filePath);
	if (source == NULL)
	{
		throw std::runtime_error("OpenGL: Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("OpenGL: Failed to compile shader.");
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
		throw std::runtime_error("OpenGL: Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("OpenGL: Failed to compile shader.");
	}
}

GLuint FragmentShader_OpenGL::GetGLShaderID() const
{
	return m_glShaderID;
}

ShaderProgram_OpenGL::ShaderProgram_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader)
	: ShaderProgram((uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment), m_activeTextureUnit(0)
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
		throw std::runtime_error("OpenGL: Shader program failed to link.");
	}

	ReflectParamLocations();
}

GLuint ShaderProgram_OpenGL::GetGLProgramID() const
{
	return m_glProgramID;
}

unsigned int ShaderProgram_OpenGL::GetParamLocation(const char* paramName) const
{
	if (m_paramLocations.find(paramName) != m_paramLocations.end())
	{
		return m_paramLocations.at(paramName);
	}
	return -1;
}

void ShaderProgram_OpenGL::UpdateParameterValue(GLuint location, EShaderParamType type, const void* value)
{
	switch (type)
	{
	case EShaderParamType::Int1:
		glUniform1i(location, *(int*)value);
		break;
	case EShaderParamType::Float1:
		glUniform1f(location, *(float*)value);
		break;
	case EShaderParamType::Vec2:
		glUniform2fv(location, 1, (float*)value);
		break;
	case EShaderParamType::Vec3:
		glUniform3fv(location, 1, (float*)value);
		break;
	case EShaderParamType::Vec4:
		glUniform4fv(location, 1, (float*)value);
		break;
	case EShaderParamType::Mat2:
		glUniformMatrix2fv(location, 1, false, (float*)value);
		break;
	case EShaderParamType::Mat3:
		glUniformMatrix3fv(location, 1, false, (float*)value);
		break;
	case EShaderParamType::Mat4:
		glUniformMatrix4fv(location, 1, false, (float*)value);
		break;
	case EShaderParamType::Texture2D:
		glActiveTexture(GL_TEXTURE0 + m_activeTextureUnit);
		glBindTexture(GL_TEXTURE_2D, *(unsigned int*)value);
		glUniform1i(location, m_activeTextureUnit);
		m_activeTextureUnit += 1;
		break;
	default:
		throw std::runtime_error("OpenGL: Unsupported shader parameter type.");
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
	m_paramLocations.emplace(ShaderParamNames::MODEL_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::MODEL_MATRIX));
	m_paramLocations.emplace(ShaderParamNames::VIEW_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::VIEW_MATRIX));
	m_paramLocations.emplace(ShaderParamNames::PROJECTION_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::PROJECTION_MATRIX));
	m_paramLocations.emplace(ShaderParamNames::NORMAL_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::NORMAL_MATRIX));

	m_paramLocations.emplace(ShaderParamNames::LIGHT_SPACE_MATRIX, glGetUniformLocation(m_glProgramID, ShaderParamNames::LIGHT_SPACE_MATRIX));
	m_paramLocations.emplace(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE));

	m_paramLocations.emplace(ShaderParamNames::CAMERA_POSITION, glGetUniformLocation(m_glProgramID, ShaderParamNames::CAMERA_POSITION));
	m_paramLocations.emplace(ShaderParamNames::CAMERA_APERTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::CAMERA_APERTURE));
	m_paramLocations.emplace(ShaderParamNames::CAMERA_FOCALDISTANCE, glGetUniformLocation(m_glProgramID, ShaderParamNames::CAMERA_FOCALDISTANCE));
	m_paramLocations.emplace(ShaderParamNames::CAMERA_IMAGEDISTANCE, glGetUniformLocation(m_glProgramID, ShaderParamNames::CAMERA_IMAGEDISTANCE));

	m_paramLocations.emplace(ShaderParamNames::TIME, glGetUniformLocation(m_glProgramID, ShaderParamNames::TIME));

	m_paramLocations.emplace(ShaderParamNames::ALBEDO_COLOR, glGetUniformLocation(m_glProgramID, ShaderParamNames::ALBEDO_COLOR));

	m_paramLocations.emplace(ShaderParamNames::ANISOTROPY, glGetUniformLocation(m_glProgramID, ShaderParamNames::ANISOTROPY));
	m_paramLocations.emplace(ShaderParamNames::ROUGHNESS, glGetUniformLocation(m_glProgramID, ShaderParamNames::ROUGHNESS));

	m_paramLocations.emplace(ShaderParamNames::ALBEDO_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::ALBEDO_TEXTURE));

	m_paramLocations.emplace(ShaderParamNames::GNORMAL_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::GNORMAL_TEXTURE));
	m_paramLocations.emplace(ShaderParamNames::GPOSITION_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::GPOSITION_TEXTURE));

	m_paramLocations.emplace(ShaderParamNames::DEPTH_TEXTURE_1, glGetUniformLocation(m_glProgramID, ShaderParamNames::DEPTH_TEXTURE_1));
	m_paramLocations.emplace(ShaderParamNames::DEPTH_TEXTURE_2, glGetUniformLocation(m_glProgramID, ShaderParamNames::DEPTH_TEXTURE_2));
	m_paramLocations.emplace(ShaderParamNames::COLOR_TEXTURE_1, glGetUniformLocation(m_glProgramID, ShaderParamNames::COLOR_TEXTURE_1));
	m_paramLocations.emplace(ShaderParamNames::COLOR_TEXTURE_2, glGetUniformLocation(m_glProgramID, ShaderParamNames::COLOR_TEXTURE_2));

	m_paramLocations.emplace(ShaderParamNames::TONE_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::TONE_TEXTURE));
	m_paramLocations.emplace(ShaderParamNames::NOISE_TEXTURE_1, glGetUniformLocation(m_glProgramID, ShaderParamNames::NOISE_TEXTURE_1));
	m_paramLocations.emplace(ShaderParamNames::MASK_TEXTURE_1, glGetUniformLocation(m_glProgramID, ShaderParamNames::MASK_TEXTURE_1));
	m_paramLocations.emplace(ShaderParamNames::NOISE_TEXTURE_2, glGetUniformLocation(m_glProgramID, ShaderParamNames::NOISE_TEXTURE_2));
	m_paramLocations.emplace(ShaderParamNames::MASK_TEXTURE_2, glGetUniformLocation(m_glProgramID, ShaderParamNames::MASK_TEXTURE_2));

	m_paramLocations.emplace(ShaderParamNames::BOOL_1, glGetUniformLocation(m_glProgramID, ShaderParamNames::BOOL_1));

	// For line drawing
	m_paramLocations.emplace(ShaderParamNames::SAMPLE_MATRIX_TEXTURE, glGetUniformLocation(m_glProgramID, ShaderParamNames::SAMPLE_MATRIX_TEXTURE));
}