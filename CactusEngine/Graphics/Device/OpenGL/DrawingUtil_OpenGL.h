#pragma once
#include "SharedTypes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>

namespace Engine
{
	inline char* ReadSourceFileAsChar(const char* shaderFile)
	{
		std::ifstream ifs(shaderFile, std::ios::in | std::ios::binary | std::ios::ate);

		if (ifs.is_open())
		{
			size_t filesize = static_cast<size_t>(ifs.tellg());
			ifs.seekg(0, std::ios::beg);
			char* bytes = new char[filesize + (size_t)1];
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
		char* logMsg = new char[logSize];
		glGetShaderInfoLog(shaderID, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;
	}

	inline void PrintProgramLinkError_GL(GLuint programID)
	{
		GLint  logSize;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog(programID, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;
	}

	inline unsigned int OpenGLFormat(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::RGBA32F:
			return GL_RGBA32F;
		case ETextureFormat::Depth:
			return GL_DEPTH_COMPONENT32F;
		case ETextureFormat::RGBA8_SRGB:
			return GL_SRGB8_ALPHA8;
		default:
			std::cerr << "Unhandled OpenGL format." << std::endl;
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
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			return GL_DEPTH_COMPONENT;
		default:
			std::cerr << "Unhandled OpenGL pixel format." << std::endl;
			break;
		}
		return 0;
	}

	inline int OpenGLDataType(EDataType type)
	{
		switch (type)
		{
		case EDataType::Float32:
			return GL_FLOAT;
		case EDataType::UByte:
			return GL_UNSIGNED_BYTE;
		default:
			std::cerr << "Unhandled OpenGL data type." << std::endl;
			break;
		}
		return -1;
	}

	inline uint32_t OpenGLTypeSize(EDataType type)
	{
		switch (type)
		{
		case EDataType::Float32:
			return 16;
		case EDataType::UByte:
			return 4;
		default:
			std::cerr << "Unhandled OpenGL data type." << std::endl;
			break;
		}
		return 0;
	}

	inline int OpenGLBlendFactor(EBlendFactor factor)
	{
		switch (factor)
		{
		case EBlendFactor::SrcAlpha:
			return GL_SRC_ALPHA;
		case EBlendFactor::OneMinusSrcAlpha:
			return GL_ONE_MINUS_SRC_ALPHA;
		default:
			std::cerr << "Unhandled OpenGL blend factor type." << std::endl;
			break;
		}
		return -1;
	}
}