#pragma once

namespace Engine
{
	namespace BuiltInResourcesPath
	{
		// Vulkan

		static const char* SHADER_VERTEX_BASIC_VK = "Assets/Shader/SPIRV/Basic_vert.spv";
		static const char* SHADER_FRAGMENT_BASIC_VK = "Assets/Shader/SPIRV/Basic_frag.spv";
		static const char* SHADER_VERTEX_BASIC_TRANSPARENT_VK = "Assets/Shader/SPIRV/Basic_Transparent_vert.spv";
		static const char* SHADER_FRAGMENT_BASIC_TRANSPARENT_VK = "Assets/Shader/SPIRV/Basic_Transparent_frag.spv";

		static const char* SHADER_VERTEX_WATER_BASIC_VK = "Assets/Shader/SPIRV/Water_Basic_vert.spv";
		static const char* SHADER_FRAGMENT_WATER_BASIC_VK = "Assets/Shader/SPIRV/Water_Basic_frag.spv";

		static const char* SHADER_VERTEX_FULLSCREEN_QUAD_VK = "Assets/Shader/SPIRV/FullScreenQuad_vert.spv";
		static const char* SHADER_FRAGMENT_DEPTH_COLORBLEND_2_VK = "Assets/Shader/SPIRV/DepthBased_ColorBlend_2_frag.spv";
		static const char* SHADER_FRAGMENT_DEPTH_OF_FIELD_VK = "Assets/Shader/SPIRV/DepthOfField_frag.spv";

		static const char* SHADER_VERTEX_GBUFFER_VK = "Assets/Shader/SPIRV/GBuffer_vert.spv";
		static const char* SHADER_FRAGMENT_GBUFFER_VK = "Assets/Shader/SPIRV/GBuffer_frag.spv";

		static const char* SHADER_VERTEX_ANIMESTYLE_VK = "Assets/Shader/SPIRV/AnimeStyle_vert.spv";
		static const char* SHADER_FRAGMENT_ANIMESTYLE_VK = "Assets/Shader/SPIRV/AnimeStyle_frag.spv";

		static const char* SHADER_VERTEX_SHADOWMAP_VK = "Assets/Shader/SPIRV/ShadowMap_vert.spv";
		static const char* SHADER_FRAGMENT_SHADOWMAP_VK = "Assets/Shader/SPIRV/ShadowMap_frag.spv";

		static const char* SHADER_VERTEX_DEFERRED_LIGHTING_VK = "Assets/Shader/SPIRV/LightDeferred_vert.spv";
		static const char* SHADER_FRAGMENT_DEFERRED_LIGHTING_VK = "Assets/Shader/SPIRV/LightDeferred_frag.spv";
		static const char* SHADER_FRAGMENT_DEFERRED_LIGHTING_DIR_VK = "Assets/Shader/SPIRV/LightDeferred_Directional_frag.spv";
	}
}