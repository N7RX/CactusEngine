#include "Shaders_GL.h"
#include "GHIUtilities_GL.h"
#include "Buffers_GL.h"
#include "Textures_GL.h"
#include "BuiltInShaderType.h"
#include "ImageTexture.h"
#include "RenderTexture.h";

namespace Engine
{
	VertexShader_GL::VertexShader_GL(const char* filePath)
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

	GLuint VertexShader_GL::GetGLShaderID() const
	{
		return m_glShaderID;
	}

	FragmentShader_GL::FragmentShader_GL(const char* filePath)
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

	GLuint FragmentShader_GL::GetGLShaderID() const
	{
		return m_glShaderID;
	}

	ShaderProgram_GL::ShaderProgram_GL(GraphicsHardwareInterface_GL* pDevice, const VertexShader_GL* pVertexShader, const FragmentShader_GL* pFragmentShader)
		: ShaderProgram((uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment)
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

	GLuint ShaderProgram_GL::GetGLProgramID() const
	{
		return m_glProgramID;
	}

	uint32_t ShaderProgram_GL::GetParamBinding(const char* paramName) const
	{
		if (m_paramBindings.find(paramName) != m_paramBindings.end())
		{
			return m_paramBindings.at(paramName);
		}
		return -1;
	}

	void ShaderProgram_GL::UpdateParameterValue(uint32_t binding, EDescriptorType type, const RawResource* pRes)
	{
		switch (type)
		{
		case EDescriptorType::UniformBuffer:
		{
			auto pBuffer = (UniformBuffer_GL*)pRes;
			glBindBufferBase(GL_UNIFORM_BUFFER, binding, pBuffer->GetGLBufferID());
			break;
		}
		case EDescriptorType::SubUniformBuffer:
		{
			auto pBuffer = (UniformBuffer_GL*)(((SubUniformBuffer*)pRes)->m_pParentBuffer); // OpenGL does not use sub uniform buffer
			glBindBufferBase(GL_UNIFORM_BUFFER, binding, pBuffer->GetGLBufferID());
			break;
		}
		case EDescriptorType::CombinedImageSampler:
		{
			Texture2D_GL* pTexture = nullptr;
			switch (((Texture2D*)pRes)->QuerySource())
			{
			case ETexture2DSource::ImageTexture:
				pTexture = (Texture2D_GL*)(((ImageTexture*)pRes)->GetTexture());
				break;

			case ETexture2DSource::RenderTexture:
				pTexture = (Texture2D_GL*)(((RenderTexture*)pRes)->GetTexture());
				break;

			case ETexture2DSource::RawDeviceTexture:
				pTexture = (Texture2D_GL*)pRes;
				break;

			default:
				throw std::runtime_error("OpenGL: Unhandled texture 2D source type.");
				return;
			}

			glActiveTexture(GL_TEXTURE0 + binding);
			glBindTexture(GL_TEXTURE_2D, pTexture->GetGLTextureID());
			break;
		}
		default:
			throw std::runtime_error("OpenGL: Unhandled shader parameter type.");
			break;
		}
	}

	void ShaderProgram_GL::ReflectParamLocations()
	{
		// Binding info cannot be retrieved at runtime, it will be manually assigned for compatibility with Vulkan

		// Combined image samplers

		m_paramBindings.emplace(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE, 0);

		m_paramBindings.emplace(ShaderParamNames::ALBEDO_TEXTURE, 1);

		m_paramBindings.emplace(ShaderParamNames::GCOLOR_TEXTURE, 20);
		m_paramBindings.emplace(ShaderParamNames::GNORMAL_TEXTURE, 2);
		m_paramBindings.emplace(ShaderParamNames::GPOSITION_TEXTURE, 3);

		m_paramBindings.emplace(ShaderParamNames::DEPTH_TEXTURE_1, 4);
		m_paramBindings.emplace(ShaderParamNames::DEPTH_TEXTURE_2, 5);

		m_paramBindings.emplace(ShaderParamNames::COLOR_TEXTURE_1, 6);
		m_paramBindings.emplace(ShaderParamNames::COLOR_TEXTURE_2, 7);

		m_paramBindings.emplace(ShaderParamNames::TONE_TEXTURE, 8);

		m_paramBindings.emplace(ShaderParamNames::NOISE_TEXTURE_1, 9);
		m_paramBindings.emplace(ShaderParamNames::NOISE_TEXTURE_2, 10);

		m_paramBindings.emplace(ShaderParamNames::MASK_TEXTURE_1, 11);
		m_paramBindings.emplace(ShaderParamNames::MASK_TEXTURE_2, 12);

		// Uniform blocks

		m_paramBindings.emplace(ShaderParamNames::TRANSFORM_MATRICES, 14);
		m_paramBindings.emplace(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX, 15);

		m_paramBindings.emplace(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES, 16);

		m_paramBindings.emplace(ShaderParamNames::CAMERA_PROPERTIES, 17);

		m_paramBindings.emplace(ShaderParamNames::SYSTEM_VARIABLES, 18);
		m_paramBindings.emplace(ShaderParamNames::CONTROL_VARIABLES, 19);

		m_paramBindings.emplace(ShaderParamNames::LIGHTSOURCE_PROPERTIES, 21);

		// Next to be: 22
	}
}