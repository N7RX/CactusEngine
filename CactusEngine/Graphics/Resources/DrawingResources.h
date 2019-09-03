#pragma once
#include "SharedTypes.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Engine
{
	class RawResourceData
	{
	public:
		RawResourceData() = default;
		virtual ~RawResourceData() = default;
	};

	class RawResource
	{
	public :
		virtual ~RawResource() = default;

		virtual uint32_t GetSize() const;
		virtual std::shared_ptr<RawResourceData> GetData() const;

		virtual void SetData(const std::shared_ptr<RawResourceData> pData, uint32_t size);

	protected:
		RawResource() = default;

	protected:
		uint32_t m_sizeInBytes;
		std::shared_ptr<RawResourceData> m_pData;
	};

	struct VertexBufferCreateInfo
	{
		int* pIndexData;
		uint32_t indexDataCount;

		float* pPositionData;
		uint32_t positionDatacount;

		float* pNormalData;
		uint32_t normalDatacount;

		float* pTexcoordData;
		uint32_t texcoordDatacount;
	};

	class VertexBuffer : public RawResource
	{
	public:
		void SetNumberOfIndices(uint32_t count);
		uint32_t GetNumberOfIndices() const;

	protected:
		uint32_t m_numberOfIndices;
	};

	class Texture2D : public RawResource
	{

	};

	enum EGLShaderParamType
	{
		eShaderParam_Int1 = 0,
		eShaderParam_Float1,
		eShaderParam_Vec2,
		eShaderParam_Vec3,
		eShaderParam_Vec4,
		eShaderParam_Mat2,
		eShaderParam_Mat3,
		eShaderParam_Mat4
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

		unsigned int GetParamLocation(const char* paramName) const;

	protected:
		ShaderProgram(uint32_t shaderStages);

	protected:
		uint32_t m_programID;
		uint32_t m_shaderStages; // This is a bitmap
		std::unordered_map<const char*, unsigned int> m_paramLocations;
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