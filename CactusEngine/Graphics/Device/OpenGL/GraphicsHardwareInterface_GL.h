#pragma once
#include "GraphicsDevice.h"
#include "MemoryAllocator.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Engine
{
	class GraphicsHardwareInterface_GL : public GraphicsDevice
	{
	public:
		~GraphicsHardwareInterface_GL();

		void Initialize() override;
		void ShutDown() override;

		ShaderProgram* CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) override;

		bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, VertexBuffer*& pOutput) override;
		bool CreateTexture2D(const Texture2DCreateInfo& createInfo, Texture2D*& pOutput) override;
		bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, FrameBuffer*& pOutput) override;	
		bool CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, UniformBuffer*& pOutput) override;

		void UpdateShaderParameter(ShaderProgram* pShaderProgram, const ShaderParameterTable* pTable, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void SetVertexBuffer(const VertexBuffer* pVertexBuffer, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void DrawFullScreenQuad(GraphicsCommandBuffer* pCommandBuffer = nullptr) override;
		void ResizeViewPort(uint32_t width, uint32_t height) override;

		EGraphicsAPIType GetGraphicsAPIType() const override;

		// Low-level functions that shouldn't be called on OpenGL device
		GraphicsCommandPool* RequestExternalCommandPool(EQueueType queueType) override;
		GraphicsCommandBuffer* RequestCommandBuffer(GraphicsCommandPool* pCommandPool) override;
		void ReturnExternalCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) override;
		GraphicsSemaphore* RequestGraphicsSemaphore(ESemaphoreWaitStage waitStage) override;

		bool CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, DataTransferBuffer*& pOutput) override;
		bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, RenderPassObject*& pOutput) override;
		bool CreateSampler(const TextureSamplerCreateInfo& createInfo, TextureSampler*& pOutput) override;
		bool CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, PipelineVertexInputState*& pOutput) override;
		bool CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, PipelineInputAssemblyState*& pOutput) override;
		bool CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, PipelineColorBlendState*& pOutput) override;
		bool CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, PipelineRasterizationState*& pOutput) override;
		bool CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, PipelineDepthStencilState*& pOutput) override;
		bool CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, PipelineMultisampleState*& pOutput) override;
		bool CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, PipelineViewportState*& pOutput) override;
		bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipelineObject*& pOutput) override;

		void TransitionImageLayout(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void TransitionImageLayout_Immediate(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) override;
		void ResizeSwapchain(uint32_t width, uint32_t height) override;

		void BindGraphicsPipeline(const GraphicsPipelineObject* pPipeline, GraphicsCommandBuffer* pCommandBuffer) override;
		void BeginRenderPass(const RenderPassObject* pRenderPass, const FrameBuffer* pFrameBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void EndRenderPass(GraphicsCommandBuffer* pCommandBuffer) override;
		void EndCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) override;
		void CommandWaitSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) override;
		void CommandSignalSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) override;

		void Present() override;
		void FlushCommands(bool waitExecution, bool flushImplicitCommands) override;
		void FlushTransferCommands(bool waitExecution) override;
		void WaitSemaphore(GraphicsSemaphore* pSemaphore) override;

		TextureSampler* GetDefaultTextureSampler(bool withDefaultAF = false) const override;
		void GetSwapchainImages(std::vector<Texture2D*>& outImages) const override;
		uint32_t GetSwapchainPresentImageIndex() const override;

		void CopyTexture2DToDataTransferBuffer(Texture2D* pSrcTexture, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyDataTransferBufferToTexture2D(DataTransferBuffer* pSrcBuffer, Texture2D* pDstTexture, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyDataTransferBuffer(DataTransferBuffer* pSrcBuffer, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer) override;
		void CopyHostDataToDataTransferBuffer(void* pData, DataTransferBuffer* pDstBuffer, size_t size) override;
		void CopyDataTransferBufferToHostDataLocation(DataTransferBuffer* pSrcBuffer, void* pDataLoc) override;

		void SetPrimitiveTopology(GLenum mode);

	private:
		void SetRenderTarget(const FrameBuffer* pFrameBuffer, const std::vector<uint32_t>& attachments);
		void SetRenderTarget(const FrameBuffer* pFrameBuffer);

	private:
		GLuint m_attributeless_vao = -1;
		GLenum m_primitiveTopologyMode = GL_TRIANGLES;
	};

	template<>
	static GraphicsDevice* CreateGraphicsDevice<EGraphicsAPIType::OpenGL>()
	{
		GraphicsHardwareInterface_GL* pDevice;
		CE_NEW(pDevice, GraphicsHardwareInterface_GL);

		if (!gpGlobal->QueryGlobalState(EGlobalStateQueryType::GLFWInit))
		{		
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				throw std::runtime_error("OpenGL: Failed to initialize GLAD");
			}
		}

		return pDevice;
	}
}