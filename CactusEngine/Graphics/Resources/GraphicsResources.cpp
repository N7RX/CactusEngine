#include "GraphicsResources.h"

namespace Engine
{
	uint32_t RawResource::m_assignedID = 0;

	RawResource::RawResource()
		: m_sizeInBytes(0),
		m_resourceID(m_assignedID++)
	{
		
	}

	uint32_t RawResource::GetResourceID() const
	{
		return m_resourceID;
	}

	uint32_t RawResource::GetSizeInBytes() const
	{
		return m_sizeInBytes;
	}

	void RawResource::MarkSizeInByte(uint32_t size)
	{
		m_sizeInBytes = size;
	}

	FrameBuffer::FrameBuffer()
		: m_width(0),
		m_height(0)
	{

	}

	uint32_t FrameBuffer::GetWidth() const
	{
		return m_width;
	}

	uint32_t FrameBuffer::GetHeight() const
	{
		return m_height;
	}

	std::vector<float> VertexBufferCreateInfo::ConvertToInterleavedData() const
	{
		uint32_t elementStride = 14;
		std::vector<float> interleavedVertices(elementStride * (size_t)(positionDataCount / 3));

		// Layout : [ position | normal | texcoord | tangent | bitengent ]

		// TODO: optimize this process

		for (uint32_t i = 0; i < positionDataCount / 3; i++)
		{
			interleavedVertices[(size_t)elementStride * i] = pPositionData[i * 3];
			interleavedVertices[(size_t)elementStride * i + 1] = pPositionData[i * 3 + 1];
			interleavedVertices[(size_t)elementStride * i + 2] = pPositionData[i * 3 + 2];
		}

		for (uint32_t i = 0; i < normalDataCount / 3; i++)
		{
			interleavedVertices[(size_t)elementStride * i + 3] = pNormalData[i * 3];
			interleavedVertices[(size_t)elementStride * i + 4] = pNormalData[i * 3 + 1];
			interleavedVertices[(size_t)elementStride * i + 5] = pNormalData[i * 3 + 2];
		}

		for (uint32_t i = 0; i < texcoordDataCount / 2; i++)
		{
			interleavedVertices[(size_t)elementStride * i + 6] = pTexcoordData[i * 2];
			interleavedVertices[(size_t)elementStride * i + 7] = pTexcoordData[i * 2 + 1];
		}

		for (uint32_t i = 0; i < tangentDataCount / 3; i++)
		{
			interleavedVertices[(size_t)elementStride * i + 8] = pTangentData[i * 3];
			interleavedVertices[(size_t)elementStride * i + 9] = pTangentData[i * 3 + 1];
			interleavedVertices[(size_t)elementStride * i + 10] = pTangentData[i * 3 + 2];
		}

		for (uint32_t i = 0; i < bitangentDataCount / 3; i++)
		{
			interleavedVertices[(size_t)elementStride * i + 11] = pBitangentData[i * 3];
			interleavedVertices[(size_t)elementStride * i + 12] = pBitangentData[i * 3 + 1];
			interleavedVertices[(size_t)elementStride * i + 13] = pBitangentData[i * 3 + 2];
		}

		return interleavedVertices;
	}

	void VertexBuffer::SetNumberOfIndices(uint32_t count)
	{
		m_numberOfIndices = count;
	}

	uint32_t VertexBuffer::GetNumberOfIndices() const
	{
		return m_numberOfIndices;
	}

	Texture2D::Texture2D(ETexture2DSource source)
		: m_source(source),
		m_height(0),
		m_width(0),
		m_type(ETextureType::SampledImage)
	{

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

	void Texture2D::SetFilePath(const char* filePath)
	{
		m_filePath = filePath;
	}

	const char* Texture2D::GetFilePath() const
	{
		return m_filePath.c_str();
	}

	ETexture2DSource Texture2D::QuerySource() const
	{
		return m_source;
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
		: Shader(EShaderType::Vertex)
	{
	}

	FragmentShader::FragmentShader()
		: Shader(EShaderType::Fragment)
	{
	}

	ShaderProgram::ShaderProgram(uint32_t shaderStages)
		: m_shaderStages(shaderStages),
		m_programID(-1),
		m_pDevice(nullptr)
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
}