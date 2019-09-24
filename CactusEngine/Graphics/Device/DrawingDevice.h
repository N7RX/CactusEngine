#pragma once
#include "DrawingResources.h"
#include "Global.h"
#include "BasicMathTypes.h"
#include <memory>

namespace Engine
{
	class DrawingDevice : std::enable_shared_from_this<DrawingDevice>
	{
	public:
		virtual ~DrawingDevice();

		virtual void Initialize() = 0;
		virtual void ShutDown() {};

		virtual std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) = 0;

		virtual bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) = 0;
		virtual bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) = 0;

		virtual void SetClearColor(Color4 color) = 0;
		virtual void ClearTarget() = 0;
		virtual void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable) = 0;
		virtual void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer) = 0;
		virtual void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex = 0, uint32_t baseVertex = 0) = 0;

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