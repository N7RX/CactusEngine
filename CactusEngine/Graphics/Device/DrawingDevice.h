#pragma once
#include "DrawingResources.h"
#include "Global.h"
#include "BasicMathTypes.h"
#include "NoCopy.h"
#include "CommandResources.h"
#include <memory>

namespace Engine
{
	class DrawingDevice : std::enable_shared_from_this<DrawingDevice>, public NoCopy
	{
	public:
		virtual ~DrawingDevice() = default;

		// Generic functions for all devices
		virtual void Initialize() = 0;
		virtual void ShutDown() = 0;

		virtual std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath) = 0;
		virtual std::shared_ptr<ShaderProgram> CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType) = 0;

		virtual bool CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput) = 0;
		virtual bool CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput) = 0;
		virtual bool CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput) = 0;
		virtual bool CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput) = 0;

		virtual void ClearRenderTarget() = 0;
		virtual void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments) = 0;
		virtual void SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer) = 0;
		virtual void SetClearColor(Color4 color) = 0;
		virtual void SetBlendState(const DeviceBlendStateInfo& blendInfo) = 0;

		virtual void UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) = 0;
		virtual void SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) = 0;
		virtual void DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) = 0;
		virtual void DrawFullScreenQuad(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer = nullptr) = 0;
		virtual void ResizeViewPort(uint32_t width, uint32_t height) = 0;

		virtual EGraphicsDeviceType GetDeviceType() const = 0;

		// For low-level devices, e.g. Vulkan

		// For multithreading
		virtual std::shared_ptr<DrawingCommandPool> RequestExternalCommandPool(EQueueType queueType, EGPUType deviceType = EGPUType::Main) = 0;
		virtual std::shared_ptr<DrawingCommandBuffer> RequestCommandBuffer(std::shared_ptr<DrawingCommandPool> pCommandPool) = 0;
		virtual void ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual std::shared_ptr<DrawingSemaphore> RequestDrawingSemaphore(EGPUType deviceType, ESemaphoreWaitStage waitStage) = 0;

		virtual bool CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, std::shared_ptr<DataTransferBuffer>& pOutput) = 0;
		virtual bool CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput) = 0;
		virtual bool CreateSampler(const TextureSamplerCreateInfo& createInfo, std::shared_ptr<TextureSampler>& pOutput) = 0;
		virtual bool CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, std::shared_ptr<PipelineVertexInputState>& pOutput) = 0;
		virtual bool CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, std::shared_ptr<PipelineInputAssemblyState>& pOutput) = 0;
		virtual bool CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, std::shared_ptr<PipelineColorBlendState>& pOutput) = 0;
		virtual bool CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, std::shared_ptr<PipelineRasterizationState>& pOutput) = 0;
		virtual bool CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, std::shared_ptr<PipelineDepthStencilState>& pOutput) = 0;
		virtual bool CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, std::shared_ptr<PipelineMultisampleState>& pOutput) = 0;
		virtual bool CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, std::shared_ptr<PipelineViewportState>& pOutput) = 0;
		virtual bool CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput) = 0;

		virtual void TransitionImageLayout(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages) = 0;
		virtual void TransitionImageLayout_Immediate(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages) = 0;
		virtual void ResizeSwapchain(uint32_t width, uint32_t height) = 0;

		virtual void BindGraphicsPipeline(const std::shared_ptr<GraphicsPipelineObject> pPipeline, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void EndRenderPass(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void EndCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void CommandWaitSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore) = 0;
		virtual void CommandSignalSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore) = 0;

		virtual void Present() = 0;
		virtual void FlushCommands(bool waitExecution, bool flushImplicitCommands, uint32_t deviceTypeFlags = (uint32_t)EGPUType::Main | (uint32_t)EGPUType::Secondary) = 0;
		virtual void FlushTransferCommands(bool waitExecution) = 0;
		virtual void WaitSemaphore(std::shared_ptr<DrawingSemaphore> pSemaphore) = 0;

		virtual std::shared_ptr<TextureSampler> GetDefaultTextureSampler(EGPUType deviceType = EGPUType::Main) const = 0;
		virtual void GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const = 0;
		virtual uint32_t GetSwapchainPresentImageIndex() const = 0;

		virtual void CopyTexture2DToDataTransferBuffer(std::shared_ptr<Texture2D> pSrcTexture, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void CopyDataTransferBufferToTexture2D(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<Texture2D> pDstTexture, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void CopyDataTransferBufferCrossDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer) = 0;
		virtual void CopyDataTransferBufferWithinDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer) = 0;
		virtual void CopyHostDataToDataTransferBuffer(void* pData, std::shared_ptr<DataTransferBuffer> pDstBuffer, size_t size) = 0;
		virtual void CopyDataTransferBufferToHostDataLocation(std::shared_ptr<DataTransferBuffer> pSrcBuffer, void* pDataLoc) = 0;

		// TODO: discard this function in the future, it's for temporary test
		virtual void ConfigureStates_Test() = 0;

	public:
		static const uint32_t ATTRIB_POSITION_LOCATION = 0;
		static const uint32_t ATTRIB_NORMAL_LOCATION = 1;
		static const uint32_t ATTRIB_TEXCOORD_LOCATION = 2;
		static const uint32_t ATTRIB_TANGENT_LOCATION = 3;
		static const uint32_t ATTRIB_BITANGENT_LOCATION = 4;
	};

	template<EGraphicsDeviceType>
	static std::shared_ptr<DrawingDevice> CreateDrawingDevice()
	{
		return nullptr;
	}
}