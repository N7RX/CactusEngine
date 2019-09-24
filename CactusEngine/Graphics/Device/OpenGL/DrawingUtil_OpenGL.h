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
			unsigned int filesize = static_cast<unsigned int>(ifs.tellg());
			ifs.seekg(0, std::ios::beg);
			char* bytes = new char[filesize + 1];
			memset(bytes, 0, filesize + 1);
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
		case eFormat_RGBA32F:
			return GL_RGBA32F;
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
			return GL_RGBA;
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
		case eDataType_Float:
			return GL_FLOAT;
		case eDataType_UnsignedByte:
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
		case eDataType_Float:
			return 4;
			break;
		case eDataType_UnsignedByte:
			return 1;
			break;
		default:
			std::cerr << "Unhandled OpenGL data type." << std::endl;
			break;
		}
		return 0;
	}
}