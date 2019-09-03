#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>

namespace Engine
{
	static char* ReadSourceFileAsChar(const char* shaderFile)
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

	void PrintShaderCompileError_GL(GLuint shaderID)
	{
		GLint logSize;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetShaderInfoLog(shaderID, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;
	}

	void PrintProgramLinkError_GL(GLuint programID)
	{
		GLint  logSize;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog(programID, logSize, NULL, logMsg);
		std::cerr << logMsg << std::endl;
		delete[] logMsg;
	}
}