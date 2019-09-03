#pragma once

namespace Engine
{
	enum EBuiltInShaderProgramType
	{
		eShaderProgram_Basic = 0,
		EBUILTINSHADERTYPE_COUNT
	};

	namespace ShaderParamNames
	{
		static const char* PVM_MATRIX = "PVM";
		static const char* ALBEDO_COLOR = "AlbedoColor";
		static const char* ALBEDO_TEXTURE = "AlbedoTexture";
	}
}