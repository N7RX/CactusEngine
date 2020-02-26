#pragma once
#include <iostream>
#include <string>

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

	// TODO: optimize the speed of the matching process, this linear search is very slow
	static const char* MatchShaderParamName(const char* cstr)
	{
		if (std::strcmp(ShaderParamNames::MODEL_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::MODEL_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::VIEW_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::VIEW_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::PROJECTION_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::PROJECTION_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::NORMAL_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::NORMAL_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::LIGHT_SPACE_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::LIGHT_SPACE_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_POSITION, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_POSITION;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_APERTURE, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_APERTURE;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_FOCALDISTANCE, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_FOCALDISTANCE;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_IMAGEDISTANCE, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_IMAGEDISTANCE;
		}
		if (std::strcmp(ShaderParamNames::TIME, cstr) == 0)
		{
			return ShaderParamNames::TIME;
		}
		if (std::strcmp(ShaderParamNames::ALBEDO_COLOR, cstr) == 0)
		{
			return ShaderParamNames::ALBEDO_COLOR;
		}
		if (std::strcmp(ShaderParamNames::ANISOTROPY, cstr) == 0)
		{
			return ShaderParamNames::ANISOTROPY;
		}
		if (std::strcmp(ShaderParamNames::ROUGHNESS, cstr) == 0)
		{
			return ShaderParamNames::ROUGHNESS;
		}
		if (std::strcmp(ShaderParamNames::ALBEDO_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::ALBEDO_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::GNORMAL_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::GNORMAL_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::GPOSITION_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::GPOSITION_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::DEPTH_TEXTURE_1, cstr) == 0)
		{
			return ShaderParamNames::DEPTH_TEXTURE_1;
		}
		if (std::strcmp(ShaderParamNames::DEPTH_TEXTURE_2, cstr) == 0)
		{
			return ShaderParamNames::DEPTH_TEXTURE_2;
		}
		if (std::strcmp(ShaderParamNames::COLOR_TEXTURE_1, cstr) == 0)
		{
			return ShaderParamNames::COLOR_TEXTURE_1;
		}
		if (std::strcmp(ShaderParamNames::COLOR_TEXTURE_2, cstr) == 0)
		{
			return ShaderParamNames::COLOR_TEXTURE_2;
		}
		if (std::strcmp(ShaderParamNames::TONE_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::TONE_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::NOISE_TEXTURE_1, cstr) == 0)
		{
			return ShaderParamNames::NOISE_TEXTURE_1;
		}
		if (std::strcmp(ShaderParamNames::NOISE_TEXTURE_2, cstr) == 0)
		{
			return ShaderParamNames::NOISE_TEXTURE_2;
		}
		if (std::strcmp(ShaderParamNames::MASK_TEXTURE_1, cstr) == 0)
		{
			return ShaderParamNames::MASK_TEXTURE_1;
		}
		if (std::strcmp(ShaderParamNames::MASK_TEXTURE_2, cstr) == 0)
		{
			return ShaderParamNames::MASK_TEXTURE_2;
		}
		if (std::strcmp(ShaderParamNames::BOOL_1, cstr) == 0)
		{
			return ShaderParamNames::BOOL_1;
		}
		if (std::strcmp(ShaderParamNames::SAMPLE_MATRIX_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::SAMPLE_MATRIX_TEXTURE;
		}

		std::cerr << "Unmatched shader parameter name: " << cstr << std::endl;
		return nullptr;
	}
}