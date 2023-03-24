#pragma once
#include "BasicMathTypes.h"
#include "LogUtility.h"

namespace Engine
{
	enum class EBuiltInShaderProgramType
	{
		NONE = -1,
		Basic = 0,
		Basic_Transparent,
		WaterBasic,
		DepthBased_ColorBlend_2,
		AnimeStyle,
		GBuffer,
		ShadowMap,
		DOF,
		DeferredLighting,
		DeferredLighting_Directional,
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

	// Uniform block structures
	// TODO: the alignment should be set according to min device uniform buffer alignment requirement

	static const size_t UNIFORM_BUFFER_ALIGNMENT_CE = 64;

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBTransformMatrices
	{
		Matrix4x4 modelMatrix;
		Matrix4x4 normalMatrix;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBCameraMatrices
	{
		Matrix4x4 viewMatrix;
		Matrix4x4 projectionMatrix;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBLightSpaceTransformMatrix
	{
		Matrix4x4 lightSpaceMatrix;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBMaterialNumericalProperties
	{
		Vector4	albedoColor;
		float	anisotropy;
		float	roughness;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBCameraProperties
	{
		Vector3	cameraPosition;
		float	aperture;
		float	focalDistance;
		float	imageDistance;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBSystemVariables
	{
		float timeInSec;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBControlVariables
	{
		uint32_t bool_1;
	};

	struct alignas(UNIFORM_BUFFER_ALIGNMENT_CE) UBLightSourceProperties
	{
		Vector4 source;
		Color4  color;
		float	intensity;
		float	radius;
	};

	namespace ShaderParamNames
	{
		// Uniform blocks

		static const char* TRANSFORM_MATRICES = "TransformMatrices";
		static const char* CAMERA_MATRICES = "CameraMatrices";
		static const char* LIGHTSPACE_TRANSFORM_MATRIX = "LightSpaceTransformMatrix";

		static const char* MATERIAL_NUMERICAL_PROPERTIES = "MaterialNumericalProperties";

		static const char* CAMERA_PROPERTIES = "CameraProperties";

		static const char* LIGHTSOURCE_PROPERTIES = "LightSourceProperties";

		static const char* SYSTEM_VARIABLES = "SystemVariables";
		static const char* CONTROL_VARIABLES = "ControlVariables";

		// Combined image samplers

		static const char* SHADOWMAP_DEPTH_TEXTURE = "ShadowMapDepthTexture";

		static const char* ALBEDO_TEXTURE = "AlbedoTexture";

		static const char* GCOLOR_TEXTURE = "GColorTexture";
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
	}

	// TODO: optimize the speed of the matching process, this linear search is very slow
	static const char* MatchShaderParamName(const char* cstr)
	{
		if (std::strcmp(ShaderParamNames::TRANSFORM_MATRICES, cstr) == 0)
		{
			return ShaderParamNames::TRANSFORM_MATRICES;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_MATRICES, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_MATRICES;
		}
		if (std::strcmp(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX, cstr) == 0)
		{
			return ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX;
		}
		if (std::strcmp(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES, cstr) == 0)
		{
			return ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES;
		}
		if (std::strcmp(ShaderParamNames::CAMERA_PROPERTIES, cstr) == 0)
		{
			return ShaderParamNames::CAMERA_PROPERTIES;
		}
		if (std::strcmp(ShaderParamNames::LIGHTSOURCE_PROPERTIES, cstr) == 0)
		{
			return ShaderParamNames::LIGHTSOURCE_PROPERTIES;
		}
		if (std::strcmp(ShaderParamNames::SYSTEM_VARIABLES, cstr) == 0)
		{
			return ShaderParamNames::SYSTEM_VARIABLES;
		}
		if (std::strcmp(ShaderParamNames::CONTROL_VARIABLES, cstr) == 0)
		{
			return ShaderParamNames::CONTROL_VARIABLES;
		}

		if (std::strcmp(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::ALBEDO_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::ALBEDO_TEXTURE;
		}
		if (std::strcmp(ShaderParamNames::GCOLOR_TEXTURE, cstr) == 0)
		{
			return ShaderParamNames::GCOLOR_TEXTURE;
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

		LOG_ERROR((std::string)"Unhandled shader parameter name: " + cstr);
		return nullptr;
	}
}