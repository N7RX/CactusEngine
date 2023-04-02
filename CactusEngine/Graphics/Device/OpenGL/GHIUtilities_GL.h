#pragma once
#include "SharedTypes.h"
#include "LogUtility.h"
#include "MemoryAllocator.h"
#include "OpenGLIncludes.h"

#include <fstream>

namespace Engine
{
	inline char* ReadSourceFileAsChar(const char* shaderFile)
	{
		std::ifstream ifs(shaderFile, std::ios::in | std::ios::binary | std::ios::ate);

		if (ifs.is_open())
		{
			size_t filesize = static_cast<size_t>(ifs.tellg());
			ifs.seekg(0, std::ios::beg);
			char* bytes;
			CE_NEW_ARRAY(bytes, char, filesize + (size_t)1);
			memset(bytes, 0, filesize + (size_t)1);
			ifs.read(bytes, filesize);
			ifs.close();
			return bytes;
		}

		return NULL;
	}

	inline void PrintShaderCompileError_GL(GLuint shaderID)
	{
		GLint logSize;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg;
		CE_NEW_ARRAY(logMsg, char, logSize);
		glGetShaderInfoLog(shaderID, logSize, NULL, logMsg);
		LOG_ERROR(logMsg);
		CE_DELETE_ARRAY(logMsg);
	}

	inline void PrintProgramLinkError_GL(GLuint programID)
	{
		GLint logSize;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg;
		CE_NEW_ARRAY(logMsg, char, logSize);
		glGetProgramInfoLog(programID, logSize, NULL, logMsg);
		LOG_ERROR(logMsg);
		CE_DELETE_ARRAY(logMsg);
	}

	inline uint32_t OpenGLFormat(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
			return GL_RGBA32F;
		case ETextureFormat::RGBA8_SRGB:
		case ETextureFormat::BGRA8_SRGB:
			return GL_SRGB8_ALPHA8;
		case ETextureFormat::D16:
			return GL_DEPTH_COMPONENT16;
		case ETextureFormat::D24:
			return GL_DEPTH_COMPONENT24;
		case ETextureFormat::D32:
			return GL_DEPTH_COMPONENT32F;
		default:
			LOG_ERROR("OpenGL: Unhandled format.");
			break;
		}
		return 0;
	}

	inline GLenum OpenGLPixelFormat(GLuint glFormat)
	{
		switch (glFormat)
		{
		case GL_RGBA32F:
		case GL_RGBA16F:
		case GL_SRGB8_ALPHA8:
			return GL_RGBA;
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			return GL_DEPTH_COMPONENT;
		default:
			LOG_ERROR("OpenGL: Unhandled pixel format.");
			break;
		}
		return 0;
	}

	inline int32_t OpenGLDataType(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
		case ETextureFormat::RGB32F:
		case ETextureFormat::D32:
			return GL_FLOAT;
		case ETextureFormat::RGBA8_SRGB:
		case ETextureFormat::BGRA8_UNORM:
			return GL_UNSIGNED_BYTE;
		}

		LOG_ERROR("OpenGL: Unsupported conversion from texture format to data type.");
		return GL_UNSIGNED_BYTE;
	}

	inline int32_t OpenGLDataType(EDataType type)
	{
		switch (type)
		{
		case EDataType::Float32:
			return GL_FLOAT;
		case EDataType::UByte:
			return GL_UNSIGNED_BYTE;
		case EDataType::SByte:
			return GL_BYTE;
		case EDataType::UShort:
			return GL_UNSIGNED_SHORT;
		case EDataType::Short:
			return GL_SHORT;
		case EDataType::UInt32:
			return GL_UNSIGNED_INT;
		case EDataType::Int32:
			return GL_INT;
		default:
			LOG_ERROR("OpenGL: Unhandled data type.");
			break;
		}
		return -1;
	}

	inline uint32_t OpenGLTypeSize(EDataType type)
	{
		switch (type)
		{
		case EDataType::Float32:
		case EDataType::UInt32:
		case EDataType::Int32:
			return 16;
		case EDataType::UByte:
			return 4;
		case EDataType::SByte:
			return 4;
		default:
			LOG_ERROR("OpenGL: Unhandled data type.");
			break;
		}
		return 0;
	}

	inline uint32_t OpenGLTypeSize(int32_t type)
	{
		switch (type)
		{
		case GL_FLOAT:
		case GL_UNSIGNED_INT:
		case GL_INT:
			return 16;
		case GL_UNSIGNED_BYTE:
		case GL_BYTE:
			return 4;
		default:
			LOG_ERROR("OpenGL: Unhandled data type.");
			break;
		}
		return 0;
	}

	inline GLenum OpenGLBlendFactor(EBlendFactor factor)
	{
		switch (factor)
		{
		case EBlendFactor::SrcAlpha:
			return GL_SRC_ALPHA;
		case EBlendFactor::OneMinusSrcAlpha:
			return GL_ONE_MINUS_SRC_ALPHA;
		case EBlendFactor::One:
			return GL_ONE;
		default:
			LOG_ERROR("OpenGL: Unhandled blend factor type.");
			break;
		}
		return -1;
	}

	inline GLenum OpenGLCullMode(ECullMode mode)
	{
		switch (mode)
		{
		case ECullMode::Front:
			return GL_FRONT;
		case ECullMode::Back:
			return GL_BACK;
		case ECullMode::FrontAndBack:
			return GL_FRONT_AND_BACK;
		default:
			LOG_ERROR("OpenGL: Unhandled cull mode.");
			break;
		}
		return -1;
	}

	inline GLenum OpenGLAssemblyTopologyMode(EAssemblyTopology mode)
	{
		switch (mode)
		{
		case EAssemblyTopology::TriangleList:
			return GL_TRIANGLES;
		case EAssemblyTopology::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		case EAssemblyTopology::TriangleFan:
			return GL_TRIANGLE_FAN;
		case EAssemblyTopology::LineList:
			return GL_LINES;
		case EAssemblyTopology::LineStrip:
			return GL_LINE_STRIP;
		case EAssemblyTopology::PointList:
			return GL_POINTS;
		default:
			LOG_ERROR("OpenGL: Unhandled assembly topology mode.");
			break;
		}
		return -1;
	}

	inline GLenum OpenGLPolygonMode(EPolygonMode mode)
	{
		switch (mode)
		{
		case EPolygonMode::Fill:
			return GL_FILL;
		case EPolygonMode::Line:
			return GL_LINE;
		case EPolygonMode::Point:
			return GL_POINT;
		default:
			LOG_ERROR("OpenGL: Unhandled polygon mode.");
			break;
		}
		return -1;
	}

	inline GLenum OpenGLTextureFilterMode(ESamplerFilterMode mode)
	{
		switch (mode)
		{
			case ESamplerFilterMode::Nearest:
				return GL_NEAREST;
			case ESamplerFilterMode::Linear:
				return GL_LINEAR;
			default:
				LOG_ERROR("OpenGL: Unhandled sampler filter mode.");
				break;
		}
		return -1;
	}
}