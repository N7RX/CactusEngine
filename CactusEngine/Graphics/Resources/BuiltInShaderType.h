#pragma once

namespace Engine
{
	enum class EBuiltInShaderProgramType
	{
		Basic = 0,
		Basic_Transparent,
		WaterBasic,
		DepthBased_ColorBlend_2,
		AnimeStyle,
		NormalOnly,
		LineDrawing_Curvature,
		LineDrawing_Color,
		LineDrawing_Blend,
		GaussianBlur,
		ShadowMap,
		DOF,
		COUNT
	};

	enum class EMaterialTextureType
	{
		Albedo = 0,
		Normal,
		Roughness,
		AO,		
		Noise,
		Tone,
		COUNT
	};

	namespace ShaderParamNames
	{
		static const char* MODEL_MATRIX = "ModelMatrix";
		static const char* VIEW_MATRIX = "ViewMatrix";
		static const char* PROJECTION_MATRIX = "ProjectionMatrix";
		static const char* NORMAL_MATRIX = "NormalMatrix";

		static const char* LIGHT_SPACE_MATRIX = "LightSpaceMatrix";
		static const char* SHADOWMAP_DEPTH_TEXTURE = "ShadowMapDepthTexture";

		static const char* CAMERA_POSITION = "CameraPosition";
		static const char* CAMERA_APERTURE = "Aperture";
		static const char* CAMERA_FOCALDISTANCE = "FocalDistance";
		static const char* CAMERA_IMAGEDISTANCE = "ImageDistance";

		static const char* TIME = "Time";

		static const char* ALBEDO_COLOR = "AlbedoColor";

		static const char* ANISOTROPY = "Anisotropy";
		static const char* ROUGHNESS = "Roughness";

		static const char* ALBEDO_TEXTURE = "AlbedoTexture";

		static const char* GNORMAL_TEXTURE = "GNormalTexture";
		static const char* GPOSITION_TEXTURE = "GPositionTexture";

		static const char* DEPTH_TEXTURE_1 = "DepthTexture_1";
		static const char* DEPTH_TEXTURE_2 = "DepthTexture_2";

		static const char* COLOR_TEXTURE_1 = "ColorTexture_1";
		static const char* COLOR_TEXTURE_2 = "ColorTexture_2";

		static const char* TONE_TEXTURE = "ToneTexture";
		static const char* NOISE_TEXTURE_1 = "NoiseTexture_1";
		static const char* NOISE_TEXTURE_2 = "NoiseTexture_2";
		static const char* MASK_TEXTURE_1 = "MaskTexture_1";
		static const char* MASK_TEXTURE_2 = "MaskTexture_2";

		static const char* BOOL_1 = "Bool_1";

		// For line drawing
		static const char* SAMPLE_MATRIX_TEXTURE = "SampleMatrixTexture";
	}
}