#pragma once

namespace Engine
{
	enum EBuiltInShaderProgramType
	{
		eShaderProgram_Basic = 0,
		eShaderProgram_WaterBasic,
		EBUILTINSHADERTYPE_COUNT
	};

	namespace ShaderParamNames
	{
		static const char* MODEL_MATRIX = "ModelMatrix";
		static const char* VIEW_MATRIX = "ViewMatrix";
		static const char* PROJECTION_MATRIX = "ProjectionMatrix";

		static const char* CAMERA_POSITION = "CameraPosition";

		static const char* TIME = "Time";

		static const char* ALBEDO_COLOR = "AlbedoColor";
		static const char* ALBEDO_TEXTURE = "AlbedoTexture";
		
	}
}