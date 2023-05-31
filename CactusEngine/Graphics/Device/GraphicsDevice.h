#pragma once
#include "Global.h"
#include "BasicMathTypes.h"
#include "NoCopy.h"
#include "GraphicsResources.h"
#include "CommandResources.h"
#include "BaseWindow.h"

namespace Engine
{
	class GraphicsDevice : public NoCopy
	{
	public:
		virtual ~GraphicsDevice();

		// Graphics device management

		virtual void Initialize();
		virtual void ShutDown();

		virtual EGraphicsAPIType GetGraphicsAPIType() const = 0;

		// Generic graphics resource management

		virtual ShaderProgram* CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) = 0;

		virtual bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, VertexBuffer*& pOutput) = 0;
		virtual bool CreateTexture2D(const Texture2DCreateInfo& createInfo, Texture2D*& pOutput) = 0;
		virtual bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, FrameBuffer*& pOutput) = 0;
		virtual bool CreateUniformBufferManager(UniformBufferManager*& pOutput) = 0;

		virtual void GenerateMipmap(Texture2D* pTexture, GraphicsCommandBuffer* pCmdBuffer) = 0;
		virtual void CopyTexture2D(Texture2D* pSrcTexture, Texture2D*pDstTexture, GraphicsCommandBuffer* pCmdBuffer) = 0;

		virtual bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, RenderPassObject*& pOutput) = 0;
		virtual bool CreateSampler(const TextureSamplerCreateInfo& createInfo, TextureSampler*& pOutput) = 0;
		virtual bool CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, PipelineVertexInputState*& pOutput) = 0;
		virtual bool CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, PipelineInputAssemblyState*& pOutput) = 0;
		virtual bool CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, PipelineColorBlendState*& pOutput) = 0;
		virtual bool CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, PipelineRasterizationState*& pOutput) = 0;
		virtual bool CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, PipelineDepthStencilState*& pOutput) = 0;
		virtual bool CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, PipelineMultisampleState*& pOutput) = 0;
		virtual bool CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, PipelineViewportState*& pOutput) = 0;
		virtual bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipelineObject*& pOutput) = 0;

		virtual void TransitionImageLayout(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) = 0;
		virtual void TransitionImageLayout(GraphicsCommandBuffer* pCommandBuffer, Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) = 0;
		virtual void TransitionImageLayout_Immediate(Texture2D* pImage, EImageLayout newLayout, uint32_t appliedStages) = 0;

		// Command and draw call management

		virtual GraphicsCommandPool* RequestExternalCommandPool(EQueueType queueType) = 0;
		virtual GraphicsCommandBuffer* RequestCommandBuffer(GraphicsCommandPool* pCommandPool) = 0;
		virtual void ReturnExternalCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void ReturnMultipleExternalCommandBuffer(std::vector<GraphicsCommandBuffer*>& commandBuffers) = 0;
		virtual GraphicsSemaphore* RequestGraphicsSemaphore(ESemaphoreWaitStage waitStage) = 0;

		virtual void BindGraphicsPipeline(const GraphicsPipelineObject* pPipeline, GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void BeginRenderPass(const RenderPassObject* pRenderPass, const FrameBuffer* pFrameBuffer, GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void EndRenderPass(GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void EndCommandBuffer(GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void CommandWaitSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) = 0;
		virtual void CommandSignalSemaphore(GraphicsCommandBuffer* pCommandBuffer, GraphicsSemaphore* pSemaphore) = 0;

		virtual void UpdateShaderParameter(ShaderProgram* pShaderProgram, const ShaderParameterTable* pTable, GraphicsCommandBuffer* pCommandBuffer = nullptr) = 0;
		virtual void SetVertexBuffer(const VertexBuffer* pVertexBuffer, GraphicsCommandBuffer* pCommandBuffer = nullptr) = 0;

		virtual void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, GraphicsCommandBuffer* pCommandBuffer = nullptr) = 0;
		virtual void DrawFullScreenQuad(GraphicsCommandBuffer* pCommandBuffer = nullptr) = 0;

		virtual void FlushCommands(bool waitExecution, bool flushImplicitCommands) = 0;
		virtual void FlushTransferCommands(bool waitExecution) = 0;
		virtual void WaitSemaphore(GraphicsSemaphore* pSemaphore) = 0;
		virtual void WaitIdle() = 0;

		virtual TextureSampler* GetTextureSampler(ESamplerAnisotropyLevel level);

		// Swapchain management

		virtual void GetSwapchainImages(std::vector<Texture2D*>& outImages) const = 0;
		virtual uint32_t GetSwapchainPresentImageIndex() const = 0;
		virtual void Present(uint32_t frameIndex) = 0;
		virtual void ResizeSwapchain(uint32_t width, uint32_t height) = 0;

		// For host-device data transfer

		virtual bool CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, DataTransferBuffer*& pOutput) = 0;
		virtual void CopyTexture2DToDataTransferBuffer(Texture2D* pSrcTexture, DataTransferBuffer* pDstBuffer, GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void CopyDataTransferBufferToTexture2D(DataTransferBuffer* pSrcBuffer, Texture2D* pDstTexture, GraphicsCommandBuffer* pCommandBuffer) = 0;
		virtual void CopyHostDataToDataTransferBuffer(void* pData, DataTransferBuffer* pDstBuffer, size_t size) = 0;
		virtual void CopyDataTransferBufferToHostDataLocation(DataTransferBuffer* pSrcBuffer, void* pDataLoc) = 0;

	protected:
		GraphicsDevice();

	private:
		void CreateDefaultSamplers();

	public:
		// Shader vertex attribute locations
		static const uint32_t ATTRIB_POSITION_LOCATION = 0;
		static const uint32_t ATTRIB_NORMAL_LOCATION = 1;
		static const uint32_t ATTRIB_TEXCOORD_LOCATION = 2;
		static const uint32_t ATTRIB_TANGENT_LOCATION = 3;

	protected:
		std::unordered_map<ESamplerAnisotropyLevel, TextureSampler*> m_DefaultSamplers;
	};

	template<EGraphicsAPIType>
	static GraphicsDevice* CreateGraphicsDevice()
	{
		return nullptr;
	}
}