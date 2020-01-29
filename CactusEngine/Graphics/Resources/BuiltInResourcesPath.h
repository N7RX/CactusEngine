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
		static const char* SHADER_FRAGMENT_LINEDRAWING_CURVATURE_OPENGL = "Assets/Shader/GLSL/LineDrawing_Curvature.frag";
		static const char* SHADER_FRAGMENT_LINEDRAWING_COLOR_OPENGL = "Assets/Shader/GLSL/LineDrawing_Color.frag";
		static const char* SHADER_FRAGMENT_LINEDRAWING_BLEND_OPENGL = "Assets/Shader/GLSL/LineDrawing_Blend.frag";
		static const char* SHADER_FRAGMENT_DEPTH_OF_FIELD_OPENGL = "Assets/Shader/GLSL/DepthOfField.frag";

		static const char* SHADER_VERTEX_NORMALONLY_OPENGL = "Assets/Shader/GLSL/NormalOnly.vert";
		static const char* SHADER_FRAGMENT_NORMALONLY_OPENGL = "Assets/Shader/GLSL/NormalOnly.frag";

		static const char* SHADER_VERTEX_ANIMESTYLE_OPENGL = "Assets/Shader/GLSL/AnimeStyle.vert";
		static const char* SHADER_FRAGMENT_ANIMESTYLE_OPENGL = "Assets/Shader/GLSL/AnimeStyle.frag";

		static const char* SHADER_VERTEX_SHADOWMAP_OPENGL = "Assets/Shader/GLSL/ShadowMap.vert";
		static const char* SHADER_FRAGMENT_SHADOWMAP_OPENGL = "Assets/Shader/GLSL/ShadowMap.frag";

		//========
		// Vulkan

		static const char* SHADER_VERTEX_BASIC_VK = "Assets/Shader/GLSL/BasicVert.spv"; // Alert: these are placeholder, files are not created yet
		static const char* SHADER_FRAGMENT_BASIC_VK = "Assets/Shader/GLSL/BasicFrag.spv";

	}
}