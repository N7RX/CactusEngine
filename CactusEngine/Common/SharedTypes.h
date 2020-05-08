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
		Light = 0x80,
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
		Standard = 0,
		RayTracing,
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
		Transfer,
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

	enum class EGPUType
	{
		Main = 0x1,
		Secondary = 0x2,
		COUNT = 2
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
		TransferColorAttachment, // For heterogeneous-GPU mode
		COUNT
	};

	enum class ETextureFormat
	{
		// For texture
		RGBA32F = 0,
		Depth,
		RGBA8_SRGB,
		BGRA8_UNORM,
		UNDEFINED,

		// For attribute input
		RGB32F,
		RG32F,

		COUNT
	};

	enum class EDataType
	{
		Float32 = 0,
		Double,
		Boolean,
		UByte,
		SByte,	
		Int32,
		UInt32,
		Int64,
		UInt64,
		Half,
		Short,
		UShort,
		COUNT
	};

	enum class EBlendFactor
	{
		SrcAlpha = 0,
		OneMinusSrcAlpha,
		One,
		COUNT
	};

	enum class EBlendOperation
	{
		Add = 0,
		Subtract,
		Min,
		Max,
		Zero,
		Src,
		Dst,
		COUNT
	};

	enum class EBuiltInMeshType
	{
		External = 0, // From file
		Plane,
		COUNT
	};

	enum class EAttachmentOperation
	{
		None = 0,
		Clear,
		Load,
		Store,
		COUNT
	};

	enum class EImageLayout
	{
		// This is strictly modeled after Vulkan specification, may not be versatile
		Undefined = 0,
		General,
		ColorAttachment,
		DepthStencilAttachment,
		DepthStencilReadOnly,
		DepthReadOnlyStencilAttachment,
		DepthAttachmentStencilReadOnly,
		ShaderReadOnly,
		TransferSrc,
		TransferDst,
		Preinitialized,
		PresentSrc,
		SharedPresent,
		COUNT
	};

	enum class EAttachmentType
	{
		Undefined = 0,
		Color,
		Depth,
		COUNT
	};

	enum class EAssemblyTopology
	{
		PointList = 0,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan,
		COUNT
	};

	enum class EPolygonMode
	{
		Fill = 0,
		Line,
		Point,
		COUNT
	};

	enum class ECullMode
	{
		None = 0,
		Front,
		Back,
		FrontAndBack,
		COUNT
	};

	enum class ECompareOperation
	{
		Never = 0,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always,
		COUNT
	};

	enum class EVertexInputRate
	{
		PerVertex = 0,
		PerInstance,
		COUNT
	};

	enum class EDescriptorType
	{
		UniformBuffer = 0,
		SubUniformBuffer, // Only for descriptor set update, does not exist in shader
		UniformTexelBuffer,
		SampledImage,
		Sampler,
		CombinedImageSampler,
		StorageImage,
		StorageBuffer,
		StorageTexelBuffer,
		COUNT
	};

	enum class ESamplerFilterMode
	{
		Nearest = 0,
		Linear,
		Cubic,
		COUNT
	};

	enum class ESamplerAddressMode
	{
		Repeat = 0,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge,
		COUNT
	};

	enum class ESamplerMipmapMode
	{
		Nearest = 0,
		Linear,
		COUNT
	};

	enum class EDataTransferBufferUsage
	{
		TransferSrc = 0x1,
		TransferDst = 0x2,
		COUNT = 2
	};

	enum class EMemoryType
	{
		CPU_Only = 0,
		CPU_To_GPU,
		GPU_Only,
		GPU_To_CPU,
		COUNT
	};

	enum class ESemaphoreWaitStage
	{
		TopOfPipeline = 0,
		ColorAttachmentOutput,
		Transfer,
		BottomOfPipeline,
		COUNT
	};
}