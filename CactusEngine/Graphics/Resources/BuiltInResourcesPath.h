#pragma once

namespace Engine
{
	namespace BuiltInResourcesPath
	{
		//========
		// OpenGL

		static const char* SHADER_VERTEX_BASIC_OPENGL = "Assets/Shader/GLSL/Basic.vert";
		static const char* SHADER_FRAGMENT_BASIC_OPENGL = "Assets/Shader/GLSL/Basic.frag";
		static const char* SHADER_FRAGMENT_BASIC_TRANSPARENT_OPENGL = "Assets/Shader/GLSL/Basic_Transparent.frag";

		static const char* SHADER_VERTEX_WATER_BASIC_OPENGL = "Assets/Shader/GLSL/Water_Basic.vert";
		static const char* SHADER_FRAGMENT_WATER_BASIC_OPENGL = "Assets/Shader/GLSL/Water_Basic.frag";

		static const char* SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL = "Assets/Shader/GLSL/FullScreenQuad.vert";
		static const char* SHADER_FRAGMENT_GAUSSIANBLUR_OPENGL = "Assets/Shader/GLSL/Gaussian.frag";
		static const char* SHADER_FRAGMENT_DEPTH_COLORBLEND_2_OPENGL = "Assets/Shader/GLSL/DepthBased_ColorBlend_2.frag";
		static const char* SHADER_FRAGMENT_LINEDRAWING_SIMPLIFIED_OPENGL = "Assets/Shader/GLSL/LineDrawing_Simplified.frag";
		static const char* SHADER_FRAGMENT_LINEDRAWING_BLEND_OPENGL = "Assets/Shader/GLSL/LineDrawing_Blend.frag";
		static const char* SHADER_FRAGMENT_DEPTH_OF_FIELD_OPENGL = "Assets/Shader/GLSL/DepthOfField.frag";

		static const char* SHADER_VERTEX_GBUFFER_OPENGL = "Assets/Shader/GLSL/GBuffer.vert";
		static const char* SHADER_FRAGMENT_GBUFFER_OPENGL = "Assets/Shader/GLSL/GBuffer.frag";

		static const char* SHADER_VERTEX_ANIMESTYLE_OPENGL = "Assets/Shader/GLSL/AnimeStyle.vert";
		static const char* SHADER_FRAGMENT_ANIMESTYLE_OPENGL = "Assets/Shader/GLSL/AnimeStyle.frag";

		static const char* SHADER_VERTEX_SHADOWMAP_OPENGL = "Assets/Shader/GLSL/ShadowMap.vert";
		static const char* SHADER_FRAGMENT_SHADOWMAP_OPENGL = "Assets/Shader/GLSL/ShadowMap.frag";

		//========
		// Vulkan

		static const char* SHADER_VERTEX_BASIC_VK = "Assets/Shader/SPIRV/Basic_vert.spv";
		static const char* SHADER_FRAGMENT_BASIC_VK = "Assets/Shader/SPIRV/Basic_frag.spv";
		static const char* SHADER_VERTEX_BASIC_TRANSPARENT_VK = "Assets/Shader/SPIRV/Basic_Transparent_vert.spv";
		static const char* SHADER_FRAGMENT_BASIC_TRANSPARENT_VK = "Assets/Shader/SPIRV/Basic_Transparent_frag.spv";

		static const char* SHADER_VERTEX_WATER_BASIC_VK = "Assets/Shader/SPIRV/Water_Basic_vert.spv";
		static const char* SHADER_FRAGMENT_WATER_BASIC_VK = "Assets/Shader/SPIRV/Water_Basic_frag.spv";

		static const char* SHADER_VERTEX_FULLSCREEN_QUAD_VK = "Assets/Shader/SPIRV/FullScreenQuad_vert.spv";
		static const char* SHADER_FRAGMENT_GAUSSIANBLUR_VK = "Assets/Shader/SPIRV/Gaussian_frag.spv";
		static const char* SHADER_FRAGMENT_DEPTH_COLORBLEND_2_VK = "Assets/Shader/SPIRV/DepthBased_ColorBlend_2_frag.spv";
		static const char* SHADER_FRAGMENT_LINEDRAWING_SIMPLIFIED_VK = "Assets/Shader/SPIRV/LineDrawing_Simplified_frag.spv";
		static const char* SHADER_FRAGMENT_LINEDRAWING_BLEND_VK = "Assets/Shader/SPIRV/LineDrawing_Blend_frag.spv";
		static const char* SHADER_FRAGMENT_DEPTH_OF_FIELD_VK = "Assets/Shader/SPIRV/DepthOfField_frag.spv";

		static const char* SHADER_VERTEX_GBUFFER_VK = "Assets/Shader/SPIRV/GBuffer_vert.spv";
		static const char* SHADER_FRAGMENT_GBUFFER_VK = "Assets/Shader/SPIRV/GBuffer_frag.spv";

		static const char* SHADER_VERTEX_ANIMESTYLE_VK = "Assets/Shader/SPIRV/AnimeStyle_vert.spv";
		static const char* SHADER_FRAGMENT_ANIMESTYLE_VK = "Assets/Shader/SPIRV/AnimeStyle_frag.spv";

		static const char* SHADER_VERTEX_SHADOWMAP_VK = "Assets/Shader/SPIRV/ShadowMap_vert.spv";
		static const char* SHADER_FRAGMENT_SHADOWMAP_VK = "Assets/Shader/SPIRV/ShadowMap_frag.spv";

	}
}