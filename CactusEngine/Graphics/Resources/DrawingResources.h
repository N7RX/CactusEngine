#pragma once
#include "SharedTypes.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Engine
{
	class DrawingDevice;

	class RawResourceData
	{
	public:
		RawResourceData() = default;
		virtual ~RawResourceData() = default;

		std::shared_ptr<void> m_pData;
		uint32_t m_sizeInBytes;
	};

	class RawResource
	{
	public:
		virtual ~RawResource() = default;

		virtual uint32_t GetResourceID() const;

		virtual uint32_t GetSize() const;
		virtual std::shared_ptr<RawResourceData> GetData() const;

		virtual void SetData(const std::shared_ptr<RawResourceData> pData, uint32_t size);

	protected:
		RawResource();

	protected:
		uint32_t m_resourceID;
		uint32_t m_sizeInBytes;
		std::shared_ptr<RawResourceData> m_pRawData;

	private:
		static uint32_t m_assignedID;
	};

	struct VertexBufferCreateInfo
	{
		int*	 pIndexData;
		uint32_t indexDataCount;

		float*	 pPositionData;
		uint32_t positionDataCount;

		float*	 pNormalData;
		uint32_t normalDataCount;

		float*	 pTexcoordData;
		uint32_t texcoordDataCount;

		float*	 pTangentData;
		uint32_t tangentDataCount;

		float*	 pBitangentData;
		uint32_t bitangentDataCount;
	};

	class VertexBuffer : public RawResource
	{
	public:
		void SetNumberOfIndices(uint32_t count);
		uint32_t GetNumberOfIndices() const;

	protected:
		uint32_t m_numberOfIndices;
	};

	struct Texture2DCreateInfo
	{
		const void*	   pTextureData;
		uint32_t	   textureWidth;
		uint32_t	   textureHeight;
		EDataType	   dataType;
		ETextureFormat format;
		ETextureType   textureType;
	};

	class Texture2D : public RawResource
	{
	public:
		virtual ~Texture2D() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		virtual uint32_t GetTextureID() const = 0;

		void SetTextureType(ETextureType type);
		ETextureType GetTextureType() const;

		void SetFilePath(const char* filePath);
		const char* GetFilePath() const;

	protected:
		uint32_t m_width;
		uint32_t m_height;

		ETextureType m_type;
		std::string m_filePath;
	};

	struct FrameBufferCreateInfo
	{
		uint32_t framebufferWidth;
		uint32_t framebufferHeight;
		std::vector<std::shared_ptr<Texture2D>> bindTextures;
	};

	class FrameBuffer : public RawResource
	{
	public:
		virtual ~FrameBuffer() = default;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		virtual uint32_t GetFrameBufferID() const = 0;

	protected:
		uint32_t m_width;
		uint32_t m_height;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;
		EShaderType GetType() const;

	protected:
		Shader(EShaderType type);

	protected:
		EShaderType m_shaderType;
	};

	class VertexShader : public Shader
	{
	protected:
		VertexShader();
	};

	class FragmentShader : public Shader
	{
	protected:
		FragmentShader();
	};

	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() = default;

		void SetProgramID(uint32_t id);

		uint32_t GetProgramID() const;
		uint32_t GetShaderStages() const;

		virtual unsigned int GetParamLocation(const char* paramName) const;
		virtual void Reset() = 0;

	protected:
		ShaderProgram(uint32_t shaderStages);

	protected:
		uint32_t m_programID;
		DrawingDevice* m_pDevice;
		uint32_t m_shaderStages; // This is a bitmap
		std::unordered_map<const char*, unsigned int> m_paramLocations; // This is only useful in OpenGL device
	};

	struct ShaderParameterTable
	{
		ShaderParameterTable() = default;
		~ShaderParameterTable()
		{
			Clear();
		}
		
		struct ShaderParameterTableEntry
		{
			ShaderParameterTableEntry(unsigned int loc, EGLShaderParamType paramType, const void* val)
				: location(loc), type(paramType), value(val)
			{
			}

			unsigned int location;
			EGLShaderParamType type;
			const void* value;
		};

		std::vector<ShaderParameterTableEntry> m_table;

		inline void AddEntry(unsigned int loc, EGLShaderParamType type, const void* val)
		{
			m_table.emplace_back(loc, type, val);
		}

		inline void Clear()
		{
			m_table.clear();
		}
	};
}