#include "DrawingResources.h"

using namespace Engine;

uint32_t RawResource::m_assignedID = 0;

RawResource::RawResource()
{
	m_resourceID = m_assignedID++; // Alert: this ID would run out under rare situation
}

uint32_t RawResource::GetResourceID() const
{
	return m_resourceID;
}

uint32_t RawResource::GetSize() const
{
	return m_sizeInBytes;
}

std::shared_ptr<RawResourceData> RawResource::GetData() const
{
	return m_pData;
}

void RawResource::SetData(const std::shared_ptr<RawResourceData> pData, uint32_t size)
{
	m_pData = pData;
	m_sizeInBytes = size;
}

uint32_t FrameBuffer::GetWidth() const
{
	return m_width;
}

uint32_t FrameBuffer::GetHeight() const
{
	return m_height;
}

void VertexBuffer::SetNumberOfIndices(uint32_t count)
{
	m_numberOfIndices = count;
}

uint32_t VertexBuffer::GetNumberOfIndices() const
{
	return m_numberOfIndices;
}

uint32_t Texture2D::GetWidth() const
{
	return m_width;
}

uint32_t Texture2D::GetHeight() const
{
	return m_height;
}

void Texture2D::SetTextureType(ETextureType type)
{
	m_type = type;
}

ETextureType Texture2D::GetTextureType() const
{
	return m_type;
}

Shader::Shader(EShaderType type)
	: m_shaderType(type)
{
}

EShaderType Shader::GetType() const
{
	return m_shaderType;
}

VertexShader::VertexShader()
	: Shader(eShader_Vertex)
{
}

FragmentShader::FragmentShader()
	: Shader(eShader_Fragment)
{
}

ShaderProgram::ShaderProgram(uint32_t shaderStages)
	: m_shaderStages(shaderStages)
{
}

void ShaderProgram::SetProgramID(uint32_t id)
{
	m_programID = id;
}

uint32_t ShaderProgram::GetProgramID() const
{
	return m_programID;
}

uint32_t ShaderProgram::GetShaderStages() const
{
	return m_shaderStages;
}

unsigned int ShaderProgram::GetParamLocation(const char* paramName) const
{
	if (m_paramLocations.find(paramName) != m_paramLocations.end())
	{
		return m_paramLocations.at(paramName);
	}
	return -1;
}