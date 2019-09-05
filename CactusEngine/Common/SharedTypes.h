#pragma once

namespace Engine
{
	enum EECSType
	{
		eECS_Entity = 0,
		eECS_Component,
		eECS_System,
		ECSTYPE_COUNT
	};

	enum EComponentType
	{
		eCompType_Transform = 0x1,
		eCompType_MeshFilter = 0x2,
		eCompType_MeshRenderer = 0x4,
		eCompType_Material = 0x8,
		eCompType_Animation = 0x10,
		eCompType_Camera = 0x20,
		eCompType_GLC = 0x40,
		ECOMPTYPE_COUNT = 7
	};

	enum ESystemType
	{
		eSystem_Drawing = 0,
		eSystem_Animation,
		//eSystem_Event,
		//eSystem_Input,
		//eSystem_Log,
		//eSystem_Audio,
		//eSystem_Physics,
		eSystem_GLCDrawing,
		ESYSTEMTYPE_COUNT
	};

	enum ERendererType
	{
		eRenderer_Forward = 0,
		eRenderer_Deferred,
		ERENDERERTYPE_COUNT
	};

	enum EShaderType
	{
		eShader_Vertex = 0x1,
		eShader_Fragment = 0x2,
		eShader_TessControl = 0x4,
		eShader_TessEvaluation = 0x8,
		eShader_Geometry = 0x10,
		eShader_Compute = 0x20,
		ESHADERTYPE_COUNT = 6
	};

	enum EQueueType
	{
		eQueue_Graphics = 0,
		eQueue_Copy,
		eQueue_Compute,
		EQUEUETYPE_COUNT
	};

	enum EConfigurationType
	{
		eConfiguration_App = 0,
		eConfiguration_Graphics,
		ECONFIGURATION_COUNT
	};

	enum EGraphicsDeviceType
	{
		eDevice_OpenGL = 0,
		eDevice_Vulkan,
		EGRAPHICSDEVICE_COUNT
	};

	enum ECameraProjectionType
	{
		eProjectionType_Perspective = 0,
		eProjectionType_Orthographic,
		EPROJECTIONTYPE_COUNT
	};

	enum EGlobalStateQueryType
	{
		eGlobalState_GLFWInit = 0,
		EGLOBALSTATEQUERYTYPE_COUNT
	};

	enum ETextureFormat
	{
		eFormat_RGBA32F = 0,
	};

	enum EDataType
	{
		eDataType_Float = 0,
		eDataType_UnsignedByte,
	};
}