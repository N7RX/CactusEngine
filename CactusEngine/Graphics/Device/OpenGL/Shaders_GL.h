#pragma once
#include "GraphicsResources.h"
#include "GraphicsHardwareInterface_GL.h"

namespace Engine
{
	class VertexShader_GL : public VertexShader
	{
	public:
		VertexShader_GL(const char* filePath);
		~VertexShader_GL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class FragmentShader_GL : public FragmentShader
	{
	public:
		FragmentShader_GL(const char* filePath);
		~FragmentShader_GL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class ShaderProgram_GL : public ShaderProgram
	{
	public:
		ShaderProgram_GL(GraphicsHardwareInterface_GL* pDevice, const VertexShader_GL* pVertexShader, const FragmentShader_GL* pFragmentShader);

		GLuint GetGLProgramID() const;

		uint32_t GetParamBinding(const char* paramName) const override;
		void Reset() override;

		void UpdateParameterValue(uint32_t binding, EDescriptorType type, const RawResource* pRes);

	private:
		void ReflectParamLocations();

	private:
		GLuint m_glProgramID;
		std::unordered_map<const char*, uint32_t> m_paramBindings;
	};
}