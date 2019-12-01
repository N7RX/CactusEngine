#pragma once
#include "DrawingDevice.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Engine
{
	class DrawingDevice_OpenGL : public DrawingDevice
	{
	public:
		~DrawingDevice_OpenGL();

		void Initialize() override;
		void ShutDown() override;

		std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) override;
		bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput) override;

		void ClearRenderTarget() override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments) override;
		void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer) override;
		void SetClearColor(Color4 color) override;
		void SetBlendState(const DeviceBlendStateInfo& blendInfo) override;
		void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable) override;
		void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex) override;
		void DrawFullScreenQuad() override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		void Present() override;

		EGraphicsDeviceType GetDeviceType() const override;

		void ConfigureStates_Test() override;

	private:
		// Shader locations
		const GLuint ATTRIB_POSITION_LOCATION = 0;
		const GLuint ATTRIB_NORMAL_LOCATION = 1;
		const GLuint ATTRIB_TEXCOORD_LOCATION = 2;
		const GLuint ATTRIB_TANGENT_LOCATION = 3;
		const GLuint ATTRIB_BITANGENT_LOCATION = 4;

		GLuint m_attributeless_vao;
	};

	template<>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice<eDevice_OpenGL>()
	{
		auto pDevice = std::make_shared<DrawingDevice_OpenGL>();

		if (!gpGlobal->QueryGlobalState(eGlobalState_GLFWInit))
		{		
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				throw std::runtime_error("Failed to initialize GLAD");
			}
		}

		return pDevice;
	}
}