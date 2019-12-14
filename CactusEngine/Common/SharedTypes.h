#pragma once

namespace Engine
{
	enum class EECSType
	{
		Entity = 0,
		Component,
		System,
		COUNT
	};

	enum class EComponentType
	{
		Transform = 0x1,
		MeshFilter = 0x2,
		MeshRenderer = 0x4,
		Material = 0x8,
		Animation = 0x10,
		Camera = 0x20,
		Script = 0x40,
		COUNT = 7
	};

	enum class ESystemType
	{
		Drawing = 0,
		Animation,
		Event,
		Input,
		Log,
		Audio,
		Physics,
		Script,
		COUNT
	};

	enum class ERendererType
	{
		Forward = 0,
		Deferred,
		COUNT
	};

	enum class EShaderType
	{
		Vertex = 0x1,
		Fragment = 0x2,
		TessControl = 0x4,
		TessEvaluation = 0x8,
		Geometry = 0x10,
		Compute = 0x20,
		COUNT = 6
	};

	enum class EQueueType
	{
		Graphics = 0,
		Copy,
		Compute,	
		COUNT,
		Present
	};

	enum class EConfigurationType
	{
		App = 0,
		Graphics,
		COUNT
	};

	enum class EGraphicsDeviceType
	{
		OpenGL = 0,
		Vulkan,
		COUNT
	};

	enum class EGLShaderParamType
	{
		Int1 = 0,
		Float1,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4,
		Texture2D
	};

	enum class ECameraProjectionType
	{
		Perspective = 0,
		Orthographic,
		COUNT
	};

	enum class EGlobalStateQueryType
	{
		GLFWInit = 0,
		COUNT
	};

	enum class ETextureType
	{
		SampledImage = 0,
		ColorAttachment,
		DepthAttachment,
		COUNT
	};

	enum class ETextureFormat
	{
		RGBA32F = 0,
		Depth,
	};

	enum class EDataType
	{
		Float = 0,
		UnsignedByte,
	};

	enum class EBlendFactor
	{
		SrcAlpha = 0,
		OneMinusSrcAlpha
	};

	enum class EBuiltInMeshType
	{
		External = 0, // From file
		Plane,
		COUNT
	};
}