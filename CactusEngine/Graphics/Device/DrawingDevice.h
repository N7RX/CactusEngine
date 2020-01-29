#pragma once
#include "DrawingResources.h"
#include "Global.h"
#include "BasicMathTypes.h"
#include "NoCopy.h"
#include <memory>

namespace Engine
{
	struct DeviceBlendStateInfo
	{
		bool enabled;
		EBlendFactor srcFactor;
		EBlendFactor dstFactor;
	};

	class DrawingDevice : std::enable_shared_from_this<DrawingDevice>, public NoCopy
	{
	public:
		virtual ~DrawingDevice() = default;

		virtual void Initialize() = 0;
		virtual void ShutDown() = 0;

		virtual std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) = 0;
		virtual std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType) = 0;

		virtual bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) = 0;
		virtual bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) = 0;
		virtual bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput) = 0;

		virtual void ClearRenderTarget() = 0;
		virtual void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments) = 0;
		virtual void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer) = 0;
		virtual void SetClearColor(Color4 color) = 0;
		virtual void SetBlendState(const DeviceBlendStateInfo& blendInfo) = 0;
		virtual void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable) = 0;
		virtual void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer) = 0;
		virtual void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex) = 0;
		virtual void DrawFullScreenQuad() = 0;
		virtual void ResizeViewPort(uint32_t width, uint32_t height) = 0;

		virtual void Present() = 0;

		virtual EGraphicsDeviceType GetDeviceType() const = 0;

		virtual void ConfigureStates_Test() = 0; // Alert: this function will be broke down in the future, this one is for temporary test
	};

	template<EGraphicsDeviceType>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice()
	{
		return nullptr;
	}
}