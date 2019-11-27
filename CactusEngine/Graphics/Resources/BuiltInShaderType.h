#pragma once

namespace Engine
{
	enum EBuiltInShaderProgramType
	{
		eShaderProgram_Basic = 0,
		eShaderProgram_WaterBasic,
		eShaderProgram_DepthBased_ColorBlend_2,
		eShaderProgram_AnimeStyle,
		eShaderProgram_NormalOnly,
		eShaderProgram_LineDrawing_Curvature,
		eShaderProgram_LineDrawing_Color,
		eShaderProgram_LineDrawing_Blend,
		eShaderProgram_GaussianBlur,
		EBUILTINSHADERTYPE_COUNT
	};

	enum EMaterialTextureType
	{
		eMaterialTexture_Albedo = 0,
		eMaterialTexture_Normal,
		eMaterialTexture_Roughness,
		eMaterialTexture_AO,		
		eMaterialTexture_Noise,
		eMaterialTexture_Tone,
		eMaterialTexture_GNormal,
		EMATERIALTEXTURETYPE_COUNT
	};

	namespace ShaderParamNames
	{
		static const char* MODEL_MATRIX = "ModelMatrix";
		static const char* VIEW_MATRIX = "ViewMatrix";
		static const char* PROJECTION_MATRIX = "ProjectionMatrix";
		static const char* NORMAL_MATRIX = "NormalMatrix";

		static const char* CAMERA_POSITION = "CameraPosition";

		static const char* TIME = "Time";

		static const char* ALBEDO_COLOR = "AlbedoColor";

		static const char* ALBEDO_TEXTURE = "AlbedoTexture";
		static const char* NOISE_TEXTURE = "NoiseTexture";

		static const char* GNORMAL_TEXTURE = "GNormalTexture";

		static const char* DEPTH_TEXTURE_1 = "DepthTexture_1";
		static const char* DEPTH_TEXTURE_2 = "DepthTexture_2";

		static const char* COLOR_TEXTURE_1 = "ColorTexture_1";
		static const char* COLOR_TEXTURE_2 = "ColorTexture_2";

		static const char* TONE_TEXTURE = "ToneTexture";

		static const char* BOOL_1 = "Bool_1";

		// For line drawing
		static const char* SAMPLE_MATRIX_TEXTURE = "SampleMatrixTexture";
	}
}