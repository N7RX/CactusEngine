#include "HPForwardRenderer_Async.h"
#include "DrawingSystem.h"
#include "AllComponents.h"
#include "Timer.h"
#include "ImageTexture.h"

// For line drawing computation
#include <Eigen/Dense>
#include <iostream>

using namespace Engine;

HPForwardRenderer_Async::HPForwardRenderer_Async(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(ERendererType::Forward, pDevice, pSystem), m_isRunning(true), m_newCommandRecorded(false)
{
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	for (unsigned int i = 0; i < ASYNC_COMPUTE_THREAD_COUNT; i++)
	{
		m_asyncComputeThread[i] = std::thread(&HPForwardRenderer_Async::ExecuteAsycnCompute, this);
	}
#endif
}

HPForwardRenderer_Async::~HPForwardRenderer_Async()
{
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	for (unsigned int i = 0; i < ASYNC_COMPUTE_THREAD_COUNT; i++)
	{
		m_asyncComputeThread[i].join();
	}
#endif
}

void HPForwardRenderer_Async::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>(m_pDevice, 4, EGPUType::Discrete);
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	m_pRenderGraph_IG = std::make_shared<RenderGraph>(m_pDevice, 1, EGPUType::Integrated);
#endif

	BuildRenderResources();

	BuildShadowMapPass();
	BuildNormalOnlyPass();
	BuildOpaquePass();
	BuildGaussianBlurPass();
	BuildLineDrawingPass();
	BuildTransparentPass();
	BuildOpaqueTranspBlendPass();
	BuildDepthOfFieldPass();
#ifndef ASYNC_COMPUTE_TEST_CPU_CE
	BuildAsyncComputePass();
#endif

	BuildRenderNodeDependencies();
}

void HPForwardRenderer_Async::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pContext = std::make_shared<RenderContext>();
	pContext->pCamera = pCamera;
	pContext->pDrawList = &drawList;
	pContext->pRenderer = this;

	// Write initial data to async compute input
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	memcpy(Shared_PositionBuffer_Async, TransformComponent::Shared_PositionBuffer_Async + 1, sizeof(Shared_PositionBuffer_Async)); // +1: skip camera transform
#else
	m_pDevice->CopyHostDataToDataTransferBuffer(TransformComponent::Shared_PositionBuffer_Async + 1, m_pAsyncComputeBuffer, sizeof(Shared_PositionBuffer_Async));
#endif
#endif

#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	// Dispatch task to CPU threads
	{
		std::lock_guard<std::mutex> guard(m_asyncComputeMutex);
		float timeInSec = Timer::Now();
		for (unsigned int i = 0; i < ASYNC_COMPUTE_THREAD_COUNT; i++)
		{
			auto pAsyncTask = std::make_shared<AsyncThreadTaskInfo>();
			pAsyncTask->beginIndex = 128 * i;
			pAsyncTask->endIndex = 128 * (i + 1);
			pAsyncTask->timeParam = timeInSec;

			m_asyncTaskQueue.Push(pAsyncTask);
		}
	}
	m_asyncComputeCv.notify_all();
#endif

#if defined(SIMULATE_DISCRETE_GPU_UNDER_PRESSURE_CE)
	static bool firstFrame = true;
	static std::shared_ptr<DrawingCommandPool> pTransferCommandPool;
	if (firstFrame)
	{
		firstFrame = false;
		pTransferCommandPool = m_pDevice->RequestExternalCommandPool(EGPUType::Discrete, EQueueType::Transfer);
	}
	for (unsigned int i = 0; i < 20; i++)
	{
		auto pTransferCommandBuffer = m_pDevice->RequestCommandBuffer(pTransferCommandPool);
		m_pDevice->CopyDataTransferBufferToTexture2D(m_pPressureSimulationBuffer, m_pPressureSimulationTexture, pTransferCommandBuffer);
		if (i == 24)
		{
			m_pPressureSimulationSemaphore = m_pDevice->RequestDrawingSemaphore(EGPUType::Discrete, ESemaphoreWaitStage::TopOfPipeline);
			m_pDevice->CommandSignalSemaphore(pTransferCommandBuffer, m_pPressureSimulationSemaphore);
		}
		m_pDevice->EndCommandBuffer(pTransferCommandBuffer);
		m_pDevice->ReturnExternalCommandBuffer(pTransferCommandBuffer);
		if ((i + 1) % 10 == 0)
		{
			m_pDevice->FlushTransferCommands(false);
		}
	}
	m_pDevice->FlushTransferCommands(false);
#endif

#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	// Dispatch task to integrated GPU
	ExecuteIntegratedGPUCycles(pContext);
#endif

	// Dispatch task to discrete GPU
	ExecuteDiscreteGPUCycles(pContext);

	ApplyAsyncComputeResults();
}

const std::shared_ptr<GraphicsPipelineObject> HPForwardRenderer_Async::GetGraphicsPipeline(EBuiltInShaderProgramType shaderType, const char* passName) const
{
	if (m_graphicsPipelines.find(shaderType) != m_graphicsPipelines.end())
	{
		auto pPipelines = m_graphicsPipelines.at(shaderType);

		if (pPipelines.find(passName) != pPipelines.end())
		{
			return pPipelines.at(passName);
		}
		else
		{
			std::cerr << "Couldn't find associated graphics pipeline with given render pass.\n";
		}
	}
	else
	{
		std::cerr << "Couldn't find associated graphics pipelines with given shader type.\n";
	}
	return nullptr;
}

void HPForwardRenderer_Async::WriteCommandRecordList(const char* pNodeName, const std::shared_ptr<DrawingCommandBuffer>& pCommandBuffer)
{
	{
		std::lock_guard<std::mutex> guard(m_commandRecordListWriteMutex);

		m_commandRecordReadyList[m_pRenderGraph->m_renderNodePriorities[pNodeName]] = pCommandBuffer;	
		m_writtenCommandPriorities.push(m_pRenderGraph->m_renderNodePriorities[pNodeName]);
		m_newCommandRecorded = true;
	}

	m_commandRecordListCv.notify_all();
}

void HPForwardRenderer_Async::WriteAsyncComputeSemaphore(std::shared_ptr<DrawingSemaphore> pSemaphore)
{
	m_pAsyncSemaphore = pSemaphore;
}

void HPForwardRenderer_Async::ExecuteDiscreteGPUCycles(std::shared_ptr<RenderContext> pContext)
{
	ResetUniformBufferSubAllocation();

	for (auto& item : m_commandRecordReadyList)
	{
		item.second = nullptr;
		m_commandRecordReadyListFlag[item.first] = false;
	}

	m_pRenderGraph->BeginRenderPassesParallel(pContext);

	// Submit async recorded command buffers by correct sequence

	unsigned int finishedNodeCount = 0;

	while (finishedNodeCount < m_pRenderGraph->GetRenderNodeCount())
	{
		std::vector<std::pair<uint32_t, std::shared_ptr<DrawingCommandBuffer>>> buffersToReturn;

		{
			std::unique_lock<std::mutex> lock(m_commandRecordListWriteMutex);
			m_commandRecordListCv.wait(lock, [this]() { return m_newCommandRecorded; });
			m_newCommandRecorded = false;

			std::vector<uint32_t> sortedQueueContents;
			std::queue<uint32_t>  continueWaitQueue; // Command buffers that shouldn't be submitted as prior dependencies have not finished

			// Eliminate dependency jump
			while (!m_writtenCommandPriorities.empty())
			{
				sortedQueueContents.emplace_back(m_writtenCommandPriorities.front());
				m_writtenCommandPriorities.pop();
			}
			std::sort(sortedQueueContents.begin(), sortedQueueContents.end());

			for (unsigned int i = 0; i < sortedQueueContents.size(); i++)
			{
				bool proceed = true;
				uint32_t currPriority = sortedQueueContents[i];
				for (uint32_t& id : m_pRenderGraph->m_nodePriorityDependencies[currPriority]) // Check if dependent nodes have finished
				{
					if (!m_commandRecordReadyListFlag[id])
					{
						continueWaitQueue.push(currPriority);
						proceed = false;
						break;
					}
				}
				if (proceed)
				{
					m_commandRecordReadyListFlag[currPriority] = true;
					m_commandRecordReadyList[currPriority]->m_debugID = currPriority;
					buffersToReturn.emplace_back(currPriority, m_commandRecordReadyList[currPriority]);
				}
			}

			while (!continueWaitQueue.empty())
			{
				m_writtenCommandPriorities.push(continueWaitQueue.front());
				continueWaitQueue.pop();
			}
		}

		if (buffersToReturn.size() > 0)
		{
			std::sort(buffersToReturn.begin(), buffersToReturn.end(),
				[](const std::pair<uint32_t, std::shared_ptr<DrawingCommandBuffer>>& lhs, std::pair<uint32_t, std::shared_ptr<DrawingCommandBuffer>>& rhs)
				{
					return lhs.first < rhs.first;
				});

			for (unsigned int i = 0; i < buffersToReturn.size(); i++)
			{
				m_pDevice->ReturnExternalCommandBuffer(buffersToReturn[i].second);
			}

			finishedNodeCount += buffersToReturn.size();
			m_pDevice->FlushCommands(false, false, (uint32_t)EGPUType::Discrete);
		}
	}
}

void HPForwardRenderer_Async::ExecuteIntegratedGPUCycles(std::shared_ptr<RenderContext> pContext)
{
	ResetUniformBufferSubAllocation_IG();
	m_pRenderGraph_IG->BeginRenderPassesParallel(pContext);
}

void HPForwardRenderer_Async::BuildRenderResources()
{
	CreateFrameTextures();
	CreateRenderPassObjects();
	CreateFrameBuffers();
	CreateUniformBuffers();
#ifndef ASYNC_COMPUTE_TEST_CPU_CE
	CreateAsyncCompueResources();
#endif
	CreatePipelineObjects();
}

void HPForwardRenderer_Async::CreateFrameTextures()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.generateMipmap = false;
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete);
	texCreateInfo.deviceType = EGPUType::Discrete;

	// Depth attachment for shadow map pass

	texCreateInfo.textureWidth = SHADOW_MAP_RESOLUTION; // Shadow map resolution
	texCreateInfo.textureHeight = SHADOW_MAP_RESOLUTION;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pShadowMapPassDepthOutput);

	// Color outputs from normal-only pass

	texCreateInfo.textureWidth = screenWidth;
	texCreateInfo.textureHeight = screenHeight;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassNormalOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassPositionOutput);

	// Depth attachment for normal-only pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;
	texCreateInfo.initialLayout = EImageLayout::DepthStencilAttachment;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pNormalOnlyPassDepthOutput);

	// Color output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassColorOutput);

	// Shadow color output from opaque pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassShadowOutput);

	// Depth output from opaque pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pOpaquePassDepthOutput);

	// Color outputs from Gaussian blur pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassHorizontalColorOutput);
	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlurPassFinalColorOutput);

	// Curvature output from line drawing pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassCurvatureOutput);

	// Blurred output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassBlurredOutput);

	// Color output from line drawing pass

	m_pDevice->CreateTexture2D(texCreateInfo, m_pLineDrawingPassColorOutput);

	// Color output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassColorOutput);

	// Depth output from transparent pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::Depth;
	texCreateInfo.textureType = ETextureType::DepthAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pTranspPassDepthOutput);

	// Color output from blend pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;	
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pBlendPassColorOutput);

	// Horizontal color output from DOF pass

	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::ColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pDOFPassHorizontalOutput);

	// Pre-calculate local parameterization matrices
	CreateLineDrawingMatrices();

	// Post effects resources
	m_pBrushMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_1.png", EGPUType::Discrete);
	m_pBrushMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/BrushStock_2.png", EGPUType::Discrete);
	m_pPencilMaskImageTexture_1 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_1.jpg", EGPUType::Discrete);
	m_pPencilMaskImageTexture_2 = std::make_shared<ImageTexture>("Assets/Textures/PencilStock_2.jpg", EGPUType::Discrete);

	m_pBrushMaskImageTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pBrushMaskImageTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pPencilMaskImageTexture_1->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
	m_pPencilMaskImageTexture_2->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
}

void HPForwardRenderer_Async::CreateRenderPassObjects()
{
	Color4 clearColor = Color4(1, 1, 1, 1); // TODO: read clear color from scene data

	// Shadow map pass
	{
		RenderPassCreateInfo shadowMapPassCreateInfo = {};
		shadowMapPassCreateInfo.deviceType = EGPUType::Discrete;
		shadowMapPassCreateInfo.clearColor = clearColor;
		shadowMapPassCreateInfo.clearDepth = 1.0f;
		shadowMapPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 0;

		shadowMapPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(shadowMapPassCreateInfo, m_pShadowMapRenderPassObject);
	}

	// Normal only pass
	{
		RenderPassCreateInfo normalOnlyPassCreateInfo = {};
		normalOnlyPassCreateInfo.deviceType = EGPUType::Discrete;
		normalOnlyPassCreateInfo.clearColor = clearColor;
		normalOnlyPassCreateInfo.clearDepth = 1.0f;
		normalOnlyPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription normalDesc = {};
		normalDesc.format = ETextureFormat::RGBA32F;
		normalDesc.sampleCount = 1;
		normalDesc.loadOp = EAttachmentOperation::Clear;
		normalDesc.storeOp = EAttachmentOperation::Store;
		normalDesc.stencilLoadOp = EAttachmentOperation::None;
		normalDesc.stencilStoreOp = EAttachmentOperation::None;
		normalDesc.initialLayout = EImageLayout::ShaderReadOnly;
		normalDesc.usageLayout = EImageLayout::ColorAttachment;
		normalDesc.finalLayout = EImageLayout::ShaderReadOnly;
		normalDesc.type = EAttachmentType::Color;
		normalDesc.index = 0;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(normalDesc);

		RenderPassAttachmentDescription positionDesc = {};
		positionDesc.format = ETextureFormat::RGBA32F;
		positionDesc.sampleCount = 1;
		positionDesc.loadOp = EAttachmentOperation::Clear;
		positionDesc.storeOp = EAttachmentOperation::Store;
		positionDesc.stencilLoadOp = EAttachmentOperation::None;
		positionDesc.stencilStoreOp = EAttachmentOperation::None;
		positionDesc.initialLayout = EImageLayout::ShaderReadOnly;
		positionDesc.usageLayout = EImageLayout::ColorAttachment;
		positionDesc.finalLayout = EImageLayout::ShaderReadOnly;
		positionDesc.type = EAttachmentType::Color;
		positionDesc.index = 1;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(positionDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 2;

		normalOnlyPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(normalOnlyPassCreateInfo, m_pNormalOnlyRenderPassObject);
	}

	// Opaque pass
	{
		RenderPassCreateInfo opaquePassCreateInfo = {};
		opaquePassCreateInfo.deviceType = EGPUType::Discrete;
		opaquePassCreateInfo.clearColor = clearColor;
		opaquePassCreateInfo.clearDepth = 1.0f;
		opaquePassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		RenderPassAttachmentDescription shadowDesc = {};
		shadowDesc.format = ETextureFormat::RGBA32F;
		shadowDesc.sampleCount = 1;
		shadowDesc.loadOp = EAttachmentOperation::Clear;
		shadowDesc.storeOp = EAttachmentOperation::Store;
		shadowDesc.stencilLoadOp = EAttachmentOperation::None;
		shadowDesc.stencilStoreOp = EAttachmentOperation::None;
		shadowDesc.initialLayout = EImageLayout::ShaderReadOnly;
		shadowDesc.usageLayout = EImageLayout::ColorAttachment;
		shadowDesc.finalLayout = EImageLayout::ShaderReadOnly;
		shadowDesc.type = EAttachmentType::Color;
		shadowDesc.index = 1;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(shadowDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly; // Will be read in later passes
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 2;

		opaquePassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(opaquePassCreateInfo, m_pOpaqueRenderPassObject);
	}

	// Blur pass
	{
		RenderPassCreateInfo blurPassCreateInfo = {};
		blurPassCreateInfo.deviceType = EGPUType::Discrete;
		blurPassCreateInfo.clearColor = clearColor;
		blurPassCreateInfo.clearDepth = 1.0f;
		blurPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		blurPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(blurPassCreateInfo, m_pBlurRenderPassObject);
	}

	// Line drawing pass
	{
		RenderPassCreateInfo lineDrawingPassCreateInfo = {};
		lineDrawingPassCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPassCreateInfo.clearColor = clearColor;
		lineDrawingPassCreateInfo.clearDepth = 1.0f;
		lineDrawingPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		lineDrawingPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(lineDrawingPassCreateInfo, m_pLineDrawingRenderPassObject);
	}

	// Transparent pass
	{
		RenderPassCreateInfo transparentPassCreateInfo = {};
		transparentPassCreateInfo.deviceType = EGPUType::Discrete;
		transparentPassCreateInfo.clearColor = clearColor;
		transparentPassCreateInfo.clearDepth = 1.0f;
		transparentPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::Clear;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		transparentPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		RenderPassAttachmentDescription depthDesc = {};
		depthDesc.format = ETextureFormat::Depth;
		depthDesc.sampleCount = 1;
		depthDesc.loadOp = EAttachmentOperation::Clear;
		depthDesc.storeOp = EAttachmentOperation::Store;
		depthDesc.stencilLoadOp = EAttachmentOperation::None;
		depthDesc.stencilStoreOp = EAttachmentOperation::None;
		depthDesc.initialLayout = EImageLayout::ShaderReadOnly;
		depthDesc.usageLayout = EImageLayout::DepthStencilAttachment;
		depthDesc.finalLayout = EImageLayout::ShaderReadOnly; // Will be read in later passes
		depthDesc.type = EAttachmentType::Depth;
		depthDesc.index = 1;

		transparentPassCreateInfo.attachmentDescriptions.emplace_back(depthDesc);

		m_pDevice->CreateRenderPassObject(transparentPassCreateInfo, m_pTranspRenderPassObject);
	}

	// Blend pass
	{
		RenderPassCreateInfo blendPassCreateInfo = {};
		blendPassCreateInfo.deviceType = EGPUType::Discrete;
		blendPassCreateInfo.clearColor = clearColor;
		blendPassCreateInfo.clearDepth = 1.0f;
		blendPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		blendPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(blendPassCreateInfo, m_pBlendRenderPassObject);
	}

	// DOF pass
	{
		RenderPassCreateInfo dofPassCreateInfo = {};
		dofPassCreateInfo.deviceType = EGPUType::Discrete;
		dofPassCreateInfo.clearColor = clearColor;
		dofPassCreateInfo.clearDepth = 1.0f;
		dofPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::RGBA32F;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::ShaderReadOnly;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::ShaderReadOnly;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		dofPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(dofPassCreateInfo, m_pDOFRenderPassObject);
	}

	// DOF vertical + Present pass
	{
		RenderPassCreateInfo presentPassCreateInfo = {};
		presentPassCreateInfo.deviceType = EGPUType::Discrete;
		presentPassCreateInfo.clearColor = clearColor;
		presentPassCreateInfo.clearDepth = 1.0f;
		presentPassCreateInfo.clearStencil = 0;

		RenderPassAttachmentDescription colorDesc = {};
		colorDesc.format = ETextureFormat::BGRA8_UNORM;
		colorDesc.sampleCount = 1;
		colorDesc.loadOp = EAttachmentOperation::None;
		colorDesc.storeOp = EAttachmentOperation::Store;
		colorDesc.stencilLoadOp = EAttachmentOperation::None;
		colorDesc.stencilStoreOp = EAttachmentOperation::None;
		colorDesc.initialLayout = EImageLayout::PresentSrc;
		colorDesc.usageLayout = EImageLayout::ColorAttachment;
		colorDesc.finalLayout = EImageLayout::PresentSrc;
		colorDesc.type = EAttachmentType::Color;
		colorDesc.index = 0;

		presentPassCreateInfo.attachmentDescriptions.emplace_back(colorDesc);

		m_pDevice->CreateRenderPassObject(presentPassCreateInfo, m_pPresentRenderPassObject);
	}
}

void HPForwardRenderer_Async::CreateFrameBuffers()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Frame buffer for shadow map pass

	FrameBufferCreateInfo shadowMapFBCreateInfo = {};
	shadowMapFBCreateInfo.attachments.emplace_back(m_pShadowMapPassDepthOutput);
	shadowMapFBCreateInfo.framebufferWidth = SHADOW_MAP_RESOLUTION;
	shadowMapFBCreateInfo.framebufferHeight = SHADOW_MAP_RESOLUTION;
	shadowMapFBCreateInfo.deviceType = EGPUType::Discrete;
	shadowMapFBCreateInfo.pRenderPass = m_pShadowMapRenderPassObject;

	m_pDevice->CreateFrameBuffer(shadowMapFBCreateInfo, m_pShadowMapPassFrameBuffer);

	// Frame buffer for normal-only pass

	FrameBufferCreateInfo normalOnlyFBCreateInfo = {};
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassNormalOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassPositionOutput);
	normalOnlyFBCreateInfo.attachments.emplace_back(m_pNormalOnlyPassDepthOutput);
	normalOnlyFBCreateInfo.framebufferWidth = screenWidth;
	normalOnlyFBCreateInfo.framebufferHeight = screenHeight;
	normalOnlyFBCreateInfo.deviceType = EGPUType::Discrete;
	normalOnlyFBCreateInfo.pRenderPass = m_pNormalOnlyRenderPassObject;

	m_pDevice->CreateFrameBuffer(normalOnlyFBCreateInfo, m_pNormalOnlyPassFrameBuffer);

	// Frame buffer for opaque pass

	FrameBufferCreateInfo opaqueFBCreateInfo = {};
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassColorOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassShadowOutput);
	opaqueFBCreateInfo.attachments.emplace_back(m_pOpaquePassDepthOutput);
	opaqueFBCreateInfo.framebufferWidth = screenWidth;
	opaqueFBCreateInfo.framebufferHeight = screenHeight;
	opaqueFBCreateInfo.deviceType = EGPUType::Discrete;
	opaqueFBCreateInfo.pRenderPass = m_pOpaqueRenderPassObject;

	m_pDevice->CreateFrameBuffer(opaqueFBCreateInfo, m_pOpaquePassFrameBuffer);

	// Frame buffers for Gaussian blur pass

	FrameBufferCreateInfo blurFBCreateInfo_Horizontal = {};
	blurFBCreateInfo_Horizontal.attachments.emplace_back(m_pBlurPassHorizontalColorOutput);
	blurFBCreateInfo_Horizontal.framebufferWidth = screenWidth;
	blurFBCreateInfo_Horizontal.framebufferHeight = screenHeight;
	blurFBCreateInfo_Horizontal.deviceType = EGPUType::Discrete;
	blurFBCreateInfo_Horizontal.pRenderPass = m_pBlurRenderPassObject; // Also compatible with line drawing pass

	FrameBufferCreateInfo blurFBCreateInfo_FinalColor = {};
	blurFBCreateInfo_FinalColor.attachments.emplace_back(m_pBlurPassFinalColorOutput);
	blurFBCreateInfo_FinalColor.framebufferWidth = screenWidth;
	blurFBCreateInfo_FinalColor.framebufferHeight = screenHeight;
	blurFBCreateInfo_FinalColor.deviceType = EGPUType::Discrete;
	blurFBCreateInfo_FinalColor.pRenderPass = m_pBlurRenderPassObject;

	m_pDevice->CreateFrameBuffer(blurFBCreateInfo_Horizontal, m_pBlurPassFrameBuffer_Horizontal);
	m_pDevice->CreateFrameBuffer(blurFBCreateInfo_FinalColor, m_pBlurPassFrameBuffer_FinalColor);

	// Frame buffers for line drawing pass

	FrameBufferCreateInfo lineDrawFBCreateInfo_Curvature = {};
	lineDrawFBCreateInfo_Curvature.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo_Curvature.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo_Curvature.framebufferHeight = screenHeight;
	lineDrawFBCreateInfo_Curvature.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Curvature.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_RidgeSearch = {};
	lineDrawFBCreateInfo_RidgeSearch.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo_RidgeSearch.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo_RidgeSearch.framebufferHeight = screenHeight;
	lineDrawFBCreateInfo_RidgeSearch.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_RidgeSearch.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_Blur_Horizontal = {};
	lineDrawFBCreateInfo_Blur_Horizontal.attachments.emplace_back(m_pLineDrawingPassCurvatureOutput);
	lineDrawFBCreateInfo_Blur_Horizontal.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo_Blur_Horizontal.framebufferHeight = screenHeight;
	lineDrawFBCreateInfo_Blur_Horizontal.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Blur_Horizontal.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_Blur_FinalColor = {};
	lineDrawFBCreateInfo_Blur_FinalColor.attachments.emplace_back(m_pLineDrawingPassBlurredOutput);
	lineDrawFBCreateInfo_Blur_FinalColor.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo_Blur_FinalColor.framebufferHeight = screenHeight;
	lineDrawFBCreateInfo_Blur_FinalColor.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_Blur_FinalColor.pRenderPass = m_pLineDrawingRenderPassObject;

	FrameBufferCreateInfo lineDrawFBCreateInfo_FinalBlend = {};
	lineDrawFBCreateInfo_FinalBlend.attachments.emplace_back(m_pLineDrawingPassColorOutput);
	lineDrawFBCreateInfo_FinalBlend.framebufferWidth = screenWidth;
	lineDrawFBCreateInfo_FinalBlend.framebufferHeight = screenHeight;
	lineDrawFBCreateInfo_FinalBlend.deviceType = EGPUType::Discrete;
	lineDrawFBCreateInfo_FinalBlend.pRenderPass = m_pLineDrawingRenderPassObject;

	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Curvature, m_pLineDrawingPassFrameBuffer_Curvature);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_RidgeSearch, m_pLineDrawingPassFrameBuffer_RidgeSearch);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Blur_Horizontal, m_pLineDrawingPassFrameBuffer_Blur_Horizontal);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_Blur_FinalColor, m_pLineDrawingPassFrameBuffer_Blur_FinalColor);
	m_pDevice->CreateFrameBuffer(lineDrawFBCreateInfo_FinalBlend, m_pLineDrawingPassFrameBuffer_FinalBlend);

	// Frame buffer for transparent pass

	FrameBufferCreateInfo transpFBCreateInfo = {};
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassColorOutput);
	transpFBCreateInfo.attachments.emplace_back(m_pTranspPassDepthOutput);
	transpFBCreateInfo.framebufferWidth = screenWidth;
	transpFBCreateInfo.framebufferHeight = screenHeight;
	transpFBCreateInfo.deviceType = EGPUType::Discrete;
	transpFBCreateInfo.pRenderPass = m_pTranspRenderPassObject;

	m_pDevice->CreateFrameBuffer(transpFBCreateInfo, m_pTranspPassFrameBuffer);

	// Frame buffer for blend pass

	FrameBufferCreateInfo blendFBCreateInfo = {};
	blendFBCreateInfo.attachments.emplace_back(m_pBlendPassColorOutput);
	blendFBCreateInfo.framebufferWidth = screenWidth;
	blendFBCreateInfo.framebufferHeight = screenHeight;
	blendFBCreateInfo.deviceType = EGPUType::Discrete;
	blendFBCreateInfo.pRenderPass = m_pBlendRenderPassObject;

	m_pDevice->CreateFrameBuffer(blendFBCreateInfo, m_pBlendPassFrameBuffer);

	// Frame buffer for DOF pass

	FrameBufferCreateInfo dofFBCreateInfo_Horizontal = {};
	dofFBCreateInfo_Horizontal.attachments.emplace_back(m_pDOFPassHorizontalOutput);
	dofFBCreateInfo_Horizontal.framebufferWidth = screenWidth;
	dofFBCreateInfo_Horizontal.framebufferHeight = screenHeight;
	dofFBCreateInfo_Horizontal.deviceType = EGPUType::Discrete;
	dofFBCreateInfo_Horizontal.pRenderPass = m_pDOFRenderPassObject;

	m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Horizontal, m_pDOFPassFrameBuffer_Horizontal);

	// Create a framebuffer arround each swapchain image

	m_pPresentFrameBuffers = std::make_shared<SwapchainFrameBuffers>();

	std::vector<std::shared_ptr<Texture2D>> swapchainImages;
	m_pDevice->GetSwapchainImages(swapchainImages);

	for (unsigned int i = 0; i < swapchainImages.size(); i++)
	{
		FrameBufferCreateInfo dofFBCreateInfo_Final = {};
		dofFBCreateInfo_Final.attachments.emplace_back(swapchainImages[i]);
		dofFBCreateInfo_Final.framebufferWidth = screenWidth;
		dofFBCreateInfo_Final.framebufferHeight = screenHeight;
		dofFBCreateInfo_Final.deviceType = EGPUType::Discrete;
		dofFBCreateInfo_Final.pRenderPass = m_pPresentRenderPassObject;

		std::shared_ptr<FrameBuffer> pFrameBuffer;
		m_pDevice->CreateFrameBuffer(dofFBCreateInfo_Final, pFrameBuffer);
		m_pPresentFrameBuffers->frameBuffers.emplace_back(pFrameBuffer);
	}
}

void HPForwardRenderer_Async::CreateUniformBuffers()
{
	UniformBufferCreateInfo ubCreateInfo = {};
	ubCreateInfo.deviceType = EGPUType::Discrete;

	ubCreateInfo.sizeInBytes = sizeof(UBTransformMatrices) * MAX_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pTransformMatrices_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBLightSpaceTransformMatrix) * MAX_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pLightSpaceTransformMatrix_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBMaterialNumericalProperties) * MAX_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pMaterialNumericalProperties_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBCameraProperties) * MAX_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pCameraPropertie_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables) * MAX_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB);

	ubCreateInfo.sizeInBytes = sizeof(UBControlVariables) * MIN_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pControlVariables_UB);

	ubCreateInfo.deviceType = EGPUType::Integrated;

	ubCreateInfo.sizeInBytes = sizeof(UBSystemVariables) * MIN_UNIFORM_SUB_ALLOCATION;
	ubCreateInfo.appliedStages = (uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment;
	m_pDevice->CreateUniformBuffer(ubCreateInfo, m_pSystemVariables_UB_IG);
}

void HPForwardRenderer_Async::CreatePipelineObjects()
{
	// TODO: break this long function down into individual pass creation functions

	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Shared states

	// Vertex input state

	VertexInputBindingDescription vertexInputBindingDesc = {};
	vertexInputBindingDesc.binding = 0;
	vertexInputBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
	vertexInputBindingDesc.inputRate = EVertexInputRate::PerVertex;

	VertexInputAttributeDescription positionAttributeDesc = {};
	positionAttributeDesc.binding = vertexInputBindingDesc.binding;
	positionAttributeDesc.location = DrawingDevice::ATTRIB_POSITION_LOCATION;
	positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
	positionAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription normalAttributeDesc = {};
	normalAttributeDesc.binding = vertexInputBindingDesc.binding;
	normalAttributeDesc.location = DrawingDevice::ATTRIB_NORMAL_LOCATION;
	normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
	normalAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription texcoordAttributeDesc = {};
	texcoordAttributeDesc.binding = vertexInputBindingDesc.binding;
	texcoordAttributeDesc.location = DrawingDevice::ATTRIB_TEXCOORD_LOCATION;
	texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
	texcoordAttributeDesc.format = ETextureFormat::RG32F;

	VertexInputAttributeDescription tangentAttributeDesc = {};
	tangentAttributeDesc.binding = vertexInputBindingDesc.binding;
	tangentAttributeDesc.location = DrawingDevice::ATTRIB_TANGENT_LOCATION;
	tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
	tangentAttributeDesc.format = ETextureFormat::RGB32F;

	VertexInputAttributeDescription bitangentAttributeDesc = {};
	bitangentAttributeDesc.binding = vertexInputBindingDesc.binding;
	bitangentAttributeDesc.location = DrawingDevice::ATTRIB_BITANGENT_LOCATION;
	bitangentAttributeDesc.offset = VertexBufferCreateInfo::bitangentOffset;
	bitangentAttributeDesc.format = ETextureFormat::RGB32F;

	PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.bindingDescs = { vertexInputBindingDesc };
	vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc, bitangentAttributeDesc };

	std::shared_ptr<PipelineVertexInputState> pVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(vertexInputStateCreateInfo, pVertexInputState);

	// For attributeless drawing

	PipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo = {};
	emptyVertexInputStateCreateInfo.bindingDescs = {};
	emptyVertexInputStateCreateInfo.attributeDescs = {};

	std::shared_ptr<PipelineVertexInputState> pEmptyVertexInputState = nullptr;
	m_pDevice->CreatePipelineVertexInputState(emptyVertexInputStateCreateInfo, pEmptyVertexInputState);

	// Input assembly states

	PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleStrip;
	inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

	std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState_Strip = nullptr;
	m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_Strip);

	inputAssemblyStateCreateInfo.topology = EAssemblyTopology::TriangleList;
	inputAssemblyStateCreateInfo.enablePrimitiveRestart = false;

	std::shared_ptr<PipelineInputAssemblyState> pInputAssemblyState_List = nullptr;
	m_pDevice->CreatePipelineInputAssemblyState(inputAssemblyStateCreateInfo, pInputAssemblyState_List);

	// Rasterization state
	
	PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.polygonMode = EPolygonMode::Fill;
	rasterizationStateCreateInfo.enableDepthClamp = false;
	rasterizationStateCreateInfo.discardRasterizerResults = false;
	rasterizationStateCreateInfo.cullMode = ECullMode::Back;
	rasterizationStateCreateInfo.frontFaceCounterClockwise = true;

	std::shared_ptr<PipelineRasterizationState> pRasterizationState = nullptr;
	m_pDevice->CreatePipelineRasterizationState(rasterizationStateCreateInfo, pRasterizationState);

	// Color blend state attachment descriptions

	AttachmentColorBlendStateDescription attachmentBlendDesc = {};
	attachmentBlendDesc.enableBlend = true;
	attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
	attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
	attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::OneMinusSrcAlpha;
	attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::OneMinusSrcAlpha;
	attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
	attachmentBlendDesc.alphaBlendOp = EBlendOperation::Add;

	AttachmentColorBlendStateDescription attachmentNoBlendDesc = {};
	attachmentBlendDesc.enableBlend = false;
	attachmentBlendDesc.srcColorBlendFactor = EBlendFactor::SrcAlpha;
	attachmentBlendDesc.srcAlphaBlendFactor = EBlendFactor::SrcAlpha;
	attachmentBlendDesc.dstColorBlendFactor = EBlendFactor::OneMinusSrcAlpha;
	attachmentBlendDesc.dstAlphaBlendFactor = EBlendFactor::OneMinusSrcAlpha;
	attachmentBlendDesc.colorBlendOp = EBlendOperation::Add;
	attachmentBlendDesc.alphaBlendOp = EBlendOperation::Add;

	// Depth stencil states

	PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.enableDepthTest = true;
	depthStencilStateCreateInfo.enableDepthWrite = true;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Less;
	depthStencilStateCreateInfo.enableStencilTest = false;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState_DepthNoStencil = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState_DepthNoStencil);

	depthStencilStateCreateInfo.enableDepthTest = false;
	depthStencilStateCreateInfo.enableDepthWrite = false;
	depthStencilStateCreateInfo.depthCompareOP = ECompareOperation::Always;

	std::shared_ptr<PipelineDepthStencilState> pDepthStencilState_NoDepthNoStencil = nullptr;
	m_pDevice->CreatePipelineDepthStencilState(depthStencilStateCreateInfo, pDepthStencilState_NoDepthNoStencil);

	// Multisample state

	PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.enableSampleShading = false;
	multisampleStateCreateInfo.sampleCount = 1;

	std::shared_ptr<PipelineMultisampleState> pMultisampleState = nullptr;
	m_pDevice->CreatePipelineMultisampleState(multisampleStateCreateInfo, pMultisampleState);

	// Viewport state

	PipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.width = screenWidth;
	viewportStateCreateInfo.height = screenHeight;

	std::shared_ptr<PipelineViewportState> pViewportState;
	m_pDevice->CreatePipelineViewportState(viewportStateCreateInfo, pViewportState);

	// Create graphics pipelines by shader program, render pass, and individual states

	// With basic shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo basicPipelineCreateInfo = {};
		basicPipelineCreateInfo.deviceType = EGPUType::Discrete;
		basicPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic);
		basicPipelineCreateInfo.pVertexInputState = pVertexInputState;
		basicPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		basicPipelineCreateInfo.pColorBlendState = pColorBlendState;
		basicPipelineCreateInfo.pRasterizationState = pRasterizationState;
		basicPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		basicPipelineCreateInfo.pMultisampleState = pMultisampleState;
		basicPipelineCreateInfo.pViewportState = pViewportState;
		basicPipelineCreateInfo.pRenderPass = m_pOpaqueRenderPassObject; // Basic shader works under opaque pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(basicPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicPipelines;
		basicPipelines[HPForwardAsyncGraphRes::PASSNAME_OPAQUE] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::Basic] = basicPipelines;
	}

	// With basic transparent shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo basicTranspPipelineCreateInfo = {};
		basicTranspPipelineCreateInfo.deviceType = EGPUType::Discrete;
		basicTranspPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::Basic_Transparent);
		basicTranspPipelineCreateInfo.pVertexInputState = pVertexInputState;
		basicTranspPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		basicTranspPipelineCreateInfo.pColorBlendState = pColorBlendState;
		basicTranspPipelineCreateInfo.pRasterizationState = pRasterizationState;
		basicTranspPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		basicTranspPipelineCreateInfo.pMultisampleState = pMultisampleState;
		basicTranspPipelineCreateInfo.pViewportState = pViewportState;
		basicTranspPipelineCreateInfo.pRenderPass = m_pTranspRenderPassObject; // Basic transparent shader works under transparent pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(basicTranspPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicTranspPipelines;
		basicTranspPipelines[HPForwardAsyncGraphRes::PASSNAME_TRANSPARENT] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::Basic_Transparent] = basicTranspPipelines;
	}

	// With water basic shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo waterBasicPipelineCreateInfo = {};
		waterBasicPipelineCreateInfo.deviceType = EGPUType::Discrete;
		waterBasicPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::WaterBasic);
		waterBasicPipelineCreateInfo.pVertexInputState = pVertexInputState;
		waterBasicPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		waterBasicPipelineCreateInfo.pColorBlendState = pColorBlendState;
		waterBasicPipelineCreateInfo.pRasterizationState = pRasterizationState;
		waterBasicPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		waterBasicPipelineCreateInfo.pMultisampleState = pMultisampleState;
		waterBasicPipelineCreateInfo.pViewportState = pViewportState;
		waterBasicPipelineCreateInfo.pRenderPass = m_pTranspRenderPassObject; // water basic shader works under transparent pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(waterBasicPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline basicWaterPipelines;
		basicWaterPipelines[HPForwardAsyncGraphRes::PASSNAME_TRANSPARENT] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::WaterBasic] = basicWaterPipelines;
	}

	// With depth based color blend shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo blendPipelineCreateInfo = {};
		blendPipelineCreateInfo.deviceType = EGPUType::Discrete;
		blendPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		blendPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState; // Full screen quad dosen't have attribute input
		blendPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		blendPipelineCreateInfo.pColorBlendState = pColorBlendState;
		blendPipelineCreateInfo.pRasterizationState = pRasterizationState;
		blendPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		blendPipelineCreateInfo.pMultisampleState = pMultisampleState;
		blendPipelineCreateInfo.pViewportState = pViewportState;
		blendPipelineCreateInfo.pRenderPass = m_pBlendRenderPassObject; // Depth based color blend shader works under blend pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(blendPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline blendPipelines;
		blendPipelines[HPForwardAsyncGraphRes::PASSNAME_BLEND] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::DepthBased_ColorBlend_2] = blendPipelines;
	}

	// With anime style shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo animeStylePipelineCreateInfo = {};
		animeStylePipelineCreateInfo.deviceType = EGPUType::Discrete;
		animeStylePipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::AnimeStyle);
		animeStylePipelineCreateInfo.pVertexInputState = pVertexInputState;
		animeStylePipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		animeStylePipelineCreateInfo.pColorBlendState = pColorBlendState;
		animeStylePipelineCreateInfo.pRasterizationState = pRasterizationState;
		animeStylePipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		animeStylePipelineCreateInfo.pMultisampleState = pMultisampleState;
		animeStylePipelineCreateInfo.pViewportState = pViewportState;
		animeStylePipelineCreateInfo.pRenderPass = m_pOpaqueRenderPassObject; // Anime style shader works under opaque pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(animeStylePipelineCreateInfo, pPipeline);

		PassGraphicsPipeline animeStylePipelines;
		animeStylePipelines[HPForwardAsyncGraphRes::PASSNAME_OPAQUE] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::AnimeStyle] = animeStylePipelines;
	}

	// With normal only shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo normalOnlyPipelineCreateInfo = {};
		normalOnlyPipelineCreateInfo.deviceType = EGPUType::Discrete;
		normalOnlyPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);
		normalOnlyPipelineCreateInfo.pVertexInputState = pVertexInputState;
		normalOnlyPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		normalOnlyPipelineCreateInfo.pColorBlendState = pColorBlendState;
		normalOnlyPipelineCreateInfo.pRasterizationState = pRasterizationState;
		normalOnlyPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		normalOnlyPipelineCreateInfo.pMultisampleState = pMultisampleState;
		normalOnlyPipelineCreateInfo.pViewportState = pViewportState;
		normalOnlyPipelineCreateInfo.pRenderPass = m_pNormalOnlyRenderPassObject; // Normal only shader works under normal only pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(normalOnlyPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline normalOnlyPipelines;
		normalOnlyPipelines[HPForwardAsyncGraphRes::PASSNAME_NORMAL_ONLY] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::NormalOnly] = normalOnlyPipelines;
	}

	// With line drawing curvature shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		lineDrawingPipelineCreateInfo.pColorBlendState = pColorBlendState;
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing curvature shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Curvature] = lineDrawingPipelines;
	}

	// With line drawing color shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		lineDrawingPipelineCreateInfo.pColorBlendState = pColorBlendState;
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing curvature shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Color] = lineDrawingPipelines;
	}

	// With line drawing blend shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo lineDrawingPipelineCreateInfo = {};
		lineDrawingPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
		lineDrawingPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		lineDrawingPipelineCreateInfo.pColorBlendState = pColorBlendState;
		lineDrawingPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Line drawing blend shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline lineDrawingPipelines;
		lineDrawingPipelines[HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::LineDrawing_Blend] = lineDrawingPipelines;
	}

	// With Gaussian blur shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo blurPipelineCreateInfo = {};
		blurPipelineCreateInfo.deviceType = EGPUType::Discrete;
		blurPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
		blurPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		blurPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		blurPipelineCreateInfo.pColorBlendState = pColorBlendState;
		blurPipelineCreateInfo.pRasterizationState = pRasterizationState;
		blurPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		blurPipelineCreateInfo.pMultisampleState = pMultisampleState;
		blurPipelineCreateInfo.pViewportState = pViewportState;
		blurPipelineCreateInfo.pRenderPass = m_pBlurRenderPassObject; // Gaussian blur shader works under blur pass

		std::shared_ptr<GraphicsPipelineObject> pBlurPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(blurPipelineCreateInfo, pBlurPipeline);

		GraphicsPipelineCreateInfo lineDrawingBlurPipelineCreateInfo = {};
		lineDrawingBlurPipelineCreateInfo.deviceType = EGPUType::Discrete;
		lineDrawingBlurPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
		lineDrawingBlurPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		lineDrawingBlurPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		lineDrawingBlurPipelineCreateInfo.pColorBlendState = pColorBlendState;
		lineDrawingBlurPipelineCreateInfo.pRasterizationState = pRasterizationState;
		lineDrawingBlurPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		lineDrawingBlurPipelineCreateInfo.pMultisampleState = pMultisampleState;
		lineDrawingBlurPipelineCreateInfo.pViewportState = pViewportState;
		lineDrawingBlurPipelineCreateInfo.pRenderPass = m_pLineDrawingRenderPassObject; // Gaussian blur shader works under line drawing pass

		std::shared_ptr<GraphicsPipelineObject> pLineDrawingBlurPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(lineDrawingBlurPipelineCreateInfo, pLineDrawingBlurPipeline);

		PassGraphicsPipeline blurPipelines;
		blurPipelines[HPForwardAsyncGraphRes::PASSNAME_BLUR] = pBlurPipeline;
		blurPipelines[HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING] = pLineDrawingBlurPipeline;

		m_graphicsPipelines[EBuiltInShaderProgramType::GaussianBlur] = blurPipelines;
	}

	// With shadow map shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		PipelineViewportStateCreateInfo shadowMapViewportStateCreateInfo = {};
		shadowMapViewportStateCreateInfo.width = SHADOW_MAP_RESOLUTION;
		shadowMapViewportStateCreateInfo.height = SHADOW_MAP_RESOLUTION;

		std::shared_ptr<PipelineViewportState> pShadowMapViewportState;
		m_pDevice->CreatePipelineViewportState(shadowMapViewportStateCreateInfo, pShadowMapViewportState);

		GraphicsPipelineCreateInfo shadowMapPipelineCreateInfo = {};
		shadowMapPipelineCreateInfo.deviceType = EGPUType::Discrete;
		shadowMapPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);
		shadowMapPipelineCreateInfo.pVertexInputState = pVertexInputState;
		shadowMapPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_List;
		shadowMapPipelineCreateInfo.pColorBlendState = pColorBlendState;
		shadowMapPipelineCreateInfo.pRasterizationState = pRasterizationState;
		shadowMapPipelineCreateInfo.pDepthStencilState = pDepthStencilState_DepthNoStencil;
		shadowMapPipelineCreateInfo.pMultisampleState = pMultisampleState;
		shadowMapPipelineCreateInfo.pViewportState = pShadowMapViewportState;
		shadowMapPipelineCreateInfo.pRenderPass = m_pShadowMapRenderPassObject; // Shadow map shader works under shadow map pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(shadowMapPipelineCreateInfo, pPipeline);

		PassGraphicsPipeline shadowMapPipelines;
		shadowMapPipelines[HPForwardAsyncGraphRes::PASSNAME_SHADOWMAP] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::ShadowMap] = shadowMapPipelines;
	}

	// With DOF shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		GraphicsPipelineCreateInfo dofPipelineCreateInfo = {};
		dofPipelineCreateInfo.deviceType = EGPUType::Discrete;
		dofPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		dofPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		dofPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		dofPipelineCreateInfo.pColorBlendState = pColorBlendState;
		dofPipelineCreateInfo.pRasterizationState = pRasterizationState;
		dofPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		dofPipelineCreateInfo.pMultisampleState = pMultisampleState;
		dofPipelineCreateInfo.pViewportState = pViewportState;
		dofPipelineCreateInfo.pRenderPass = m_pDOFRenderPassObject; // DOF shader works under DOF pass

		std::shared_ptr<GraphicsPipelineObject> pDOFHorizontalPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(dofPipelineCreateInfo, pDOFHorizontalPipeline);

		GraphicsPipelineCreateInfo presentPipelineCreateInfo = {};
		presentPipelineCreateInfo.deviceType = EGPUType::Discrete;
		presentPipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
		presentPipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		presentPipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		presentPipelineCreateInfo.pColorBlendState = pColorBlendState;
		presentPipelineCreateInfo.pRasterizationState = pRasterizationState;
		presentPipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		presentPipelineCreateInfo.pMultisampleState = pMultisampleState;
		presentPipelineCreateInfo.pViewportState = pViewportState;
		presentPipelineCreateInfo.pRenderPass = m_pPresentRenderPassObject; // DOF shader works under DOF vertical + present pass

		std::shared_ptr<GraphicsPipelineObject> pDOFPresentPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(presentPipelineCreateInfo, pDOFPresentPipeline);

		PassGraphicsPipeline dofPipelines;
		dofPipelines[HPForwardAsyncGraphRes::PASSNAME_DOF] = pDOFHorizontalPipeline;
		dofPipelines[HPForwardAsyncGraphRes::PASSNAME_PRESENT] = pDOFPresentPipeline;

		m_graphicsPipelines[EBuiltInShaderProgramType::DOF] = dofPipelines;
	}

#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	// None
#else
	// With async compute shader
	{
		PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.blendStateDescs.push_back(attachmentNoBlendDesc);

		std::shared_ptr<PipelineColorBlendState> pColorBlendState = nullptr;
		m_pDevice->CreatePipelineColorBlendState(colorBlendStateCreateInfo, pColorBlendState);

		PipelineViewportStateCreateInfo asyncComputeViewportStateCreateInfo = {};
		asyncComputeViewportStateCreateInfo.width = 32;
		asyncComputeViewportStateCreateInfo.height = 16;

		std::shared_ptr<PipelineViewportState> pAsyncComputeViewportState;
		m_pDevice->CreatePipelineViewportState(asyncComputeViewportStateCreateInfo, pAsyncComputeViewportState);

		GraphicsPipelineCreateInfo asyncComputePipelineCreateInfo = {};
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
		asyncComputePipelineCreateInfo.deviceType = EGPUType::Discrete;
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
		asyncComputePipelineCreateInfo.deviceType = EGPUType::Integrated;
#endif
		asyncComputePipelineCreateInfo.pShaderProgram = GetDrawingSystem()->GetShaderProgramByType(EBuiltInShaderProgramType::AsyncCompute);
		asyncComputePipelineCreateInfo.pVertexInputState = pEmptyVertexInputState;
		asyncComputePipelineCreateInfo.pInputAssemblyState = pInputAssemblyState_Strip;
		asyncComputePipelineCreateInfo.pColorBlendState = pColorBlendState;
		asyncComputePipelineCreateInfo.pRasterizationState = pRasterizationState;
		asyncComputePipelineCreateInfo.pDepthStencilState = pDepthStencilState_NoDepthNoStencil;
		asyncComputePipelineCreateInfo.pMultisampleState = pMultisampleState;
		asyncComputePipelineCreateInfo.pViewportState = pAsyncComputeViewportState;
		asyncComputePipelineCreateInfo.pRenderPass = m_pAsyncComputeRenderPassObject; // Async compute shader works under async compute pass

		std::shared_ptr<GraphicsPipelineObject> pPipeline = nullptr;
		m_pDevice->CreateGraphicsPipelineObject(asyncComputePipelineCreateInfo, pPipeline);

		PassGraphicsPipeline asyncComputePipelines;
		asyncComputePipelines[HPForwardAsyncGraphRes::PASSNAME_ASYNC] = pPipeline;
		m_graphicsPipelines[EBuiltInShaderProgramType::AsyncCompute] = asyncComputePipelines;
	}
#endif
#endif
}

void HPForwardRenderer_Async::CreateAsyncCompueResources()
{
	uint32_t textureWidth = 32;
	uint32_t textureHeight = 16;

	// Data transfer buffer

	DataTransferBufferCreateInfo createInfo = {};
	createInfo.size = textureWidth * textureHeight * 16U;
	createInfo.cpuMapped = true;
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	createInfo.deviceType = EGPUType::Discrete;
	createInfo.memoryType = EMemoryType::CPU_To_GPU;
	createInfo.usageFlags = (uint32_t)EDataTransferBufferUsage::TransferSrc | (uint32_t)EDataTransferBufferUsage::TransferDst;
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	createInfo.deviceType = EGPUType::Integrated;
	createInfo.memoryType = EMemoryType::CPU_To_GPU;
	createInfo.usageFlags = (uint32_t)EDataTransferBufferUsage::TransferSrc | (uint32_t)EDataTransferBufferUsage::TransferDst;
#endif
	m_pDevice->CreateDataTransferBuffer(createInfo, m_pAsyncComputeBuffer);

	// Render pass

	RenderPassCreateInfo asyncPassCreateInfo = {};
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	asyncPassCreateInfo.deviceType = EGPUType::Discrete;
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	asyncPassCreateInfo.deviceType = EGPUType::Integrated;
#endif
	asyncPassCreateInfo.clearColor = Color4(0);
	asyncPassCreateInfo.clearDepth = 1.0f;
	asyncPassCreateInfo.clearStencil = 0;

	RenderPassAttachmentDescription outputDesc = {};
	outputDesc.format = ETextureFormat::RGBA32F;
	outputDesc.sampleCount = 1;
	outputDesc.loadOp = EAttachmentOperation::Clear;
	outputDesc.storeOp = EAttachmentOperation::Store;
	outputDesc.stencilLoadOp = EAttachmentOperation::None;
	outputDesc.stencilStoreOp = EAttachmentOperation::None;
	outputDesc.initialLayout = EImageLayout::TransferSrc;
	outputDesc.usageLayout = EImageLayout::ColorAttachment;
	outputDesc.finalLayout = EImageLayout::TransferSrc;
	outputDesc.type = EAttachmentType::Color;
	outputDesc.index = 0;

	asyncPassCreateInfo.attachmentDescriptions.emplace_back(outputDesc);

	m_pDevice->CreateRenderPassObject(asyncPassCreateInfo, m_pAsyncComputeRenderPassObject);

	// Textures

	Texture2DCreateInfo texCreateInfo = {};
	texCreateInfo.generateMipmap = false;
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete);
	texCreateInfo.deviceType = EGPUType::Discrete;
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	texCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler(EGPUType::Integrated);
	texCreateInfo.deviceType = EGPUType::Integrated;
#endif
	texCreateInfo.textureWidth = textureWidth;
	texCreateInfo.textureHeight = textureHeight;
	texCreateInfo.dataType = EDataType::Float32;
	texCreateInfo.format = ETextureFormat::RGBA32F;
	texCreateInfo.textureType = ETextureType::SampledImage;
	texCreateInfo.initialLayout = EImageLayout::ShaderReadOnly;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pAsyncComputeInputTexture);

	texCreateInfo.textureType = ETextureType::TransferColorAttachment;
	texCreateInfo.initialLayout = EImageLayout::TransferSrc;

	m_pDevice->CreateTexture2D(texCreateInfo, m_pAsyncComputeOutputTexture);

	// Frame buffers

	FrameBufferCreateInfo asyncComputeFBCreateInfo = {};

	asyncComputeFBCreateInfo.attachments.emplace_back(m_pAsyncComputeOutputTexture);
	asyncComputeFBCreateInfo.framebufferWidth = textureWidth;
	asyncComputeFBCreateInfo.framebufferHeight = textureHeight;
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	asyncComputeFBCreateInfo.deviceType = EGPUType::Discrete;
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	asyncComputeFBCreateInfo.deviceType = EGPUType::Integrated;
#endif
	asyncComputeFBCreateInfo.pRenderPass = m_pAsyncComputeRenderPassObject;

	m_pDevice->CreateFrameBuffer(asyncComputeFBCreateInfo, m_pAsyncComputePassFrameBuffer);

#if defined(SIMULATE_DISCRETE_GPU_UNDER_PRESSURE_CE)
	DataTransferBufferCreateInfo pbCreateInfo = {};
	pbCreateInfo.size = 1024 * 1024 * 4U;
	pbCreateInfo.cpuMapped = false;
	pbCreateInfo.deviceType = EGPUType::Discrete;
	pbCreateInfo.memoryType = EMemoryType::CPU_To_GPU;
	pbCreateInfo.usageFlags = (uint32_t)EDataTransferBufferUsage::TransferSrc;

	m_pDevice->CreateDataTransferBuffer(pbCreateInfo, m_pPressureSimulationBuffer);

	Texture2DCreateInfo ptTexCreateInfo = {};
	ptTexCreateInfo.generateMipmap = false;
	ptTexCreateInfo.pSampler = m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete);
	ptTexCreateInfo.deviceType = EGPUType::Discrete;
	ptTexCreateInfo.textureWidth = 1024;
	ptTexCreateInfo.textureHeight = 1024;
	ptTexCreateInfo.dataType = EDataType::UByte;
	ptTexCreateInfo.format = ETextureFormat::RGBA8_SRGB;
	ptTexCreateInfo.textureType = ETextureType::SampledImage;
	ptTexCreateInfo.initialLayout = EImageLayout::TransferDst;

	m_pDevice->CreateTexture2D(ptTexCreateInfo, m_pPressureSimulationTexture);
#endif
}

void HPForwardRenderer_Async::BuildShadowMapPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_SHADOWMAP, m_pShadowMapPassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::RPO_SHADOWMAP, m_pShadowMapRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);

	auto pShadowMapPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			Vector3   lightDir(0.0f, 0.8660254f * 16, -0.5f * 16);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -30.0f, 30.0f);
			Matrix4x4 lightView = glm::lookAt(lightDir, Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			// Set pipline and render pass
			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::ShadowMap, HPForwardAsyncGraphRes::PASSNAME_SHADOWMAP), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_SHADOWMAP)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_SHADOWMAP)), pCommandBuffer);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::ShadowMap);

			UBTransformMatrices ubTransformMatrices;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));
			
			auto pSubLightSpaceTransformMatrixUB = pLightSpaceTransformMatrixUB->AllocateSubBuffer(sizeof(UBLightSpaceTransformMatrix));
			ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
			pSubLightSpaceTransformMatrixUB->UpdateSubBufferData(&ubLightSpaceTransformMatrix);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

				auto pSubTransformMatricesUB = pTransformMatricesUB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);

				auto subMeshes = pMesh->GetSubMeshes();
				for (unsigned int i = 0; i < subMeshes->size(); ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::SubUniformBuffer, pSubLightSpaceTransformMatrixUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
				}
			}

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_SHADOWMAP, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_SHADOWMAP, pShadowMapPass);
}

void HPForwardRenderer_Async::BuildNormalOnlyPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_NORMALONLY, m_pNormalOnlyPassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::RPO_NORMALONLY, m_pNormalOnlyRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passOutput.Add(HPForwardAsyncGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);

	auto pNormalOnlyPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::NormalOnly, HPForwardAsyncGraphRes::PASSNAME_NORMAL_ONLY), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_NORMALONLY)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_NORMALONLY)), pCommandBuffer);

			// Use normal-only shader for all meshes. Alert: This will invalidate vertex shader animation
			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::NormalOnly);

			UBTransformMatrices ubTransformMatrices;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			for (auto& entity : *pContext->pDrawList)
			{
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				Matrix4x4 modelMat = pTransformComp->GetModelMatrix();
				Matrix3x3 normalMat = pTransformComp->GetNormalMatrix();

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();

				auto pSubTransformMatricesUB = pTransformMatricesUB->AllocateSubBuffer(sizeof(UBTransformMatrices));

				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);
				pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);

				pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

				auto subMeshes = pMesh->GetSubMeshes();
				for (size_t i = 0; i < subMeshes->size(); ++i)
				{
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
				}
			}

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_NORMALONLY, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_NORMALONLY, pNormalOnlyPass);
}

void HPForwardRenderer_Async::BuildOpaquePass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_OPAQUE, m_pOpaquePassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::TX_NORMALONLY_NORMAL, m_pNormalOnlyPassNormalOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_SHADOWMAP_DEPTH, m_pShadowMapPassDepthOutput);
	passInput.Add(HPForwardAsyncGraphRes::RPO_OPAQUE, m_pOpaqueRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX, m_pLightSpaceTransformMatrix_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passOutput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passOutput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);

	auto pOpaquePass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			Vector3   lightDir(0.0f, 0.8660254f, -0.5f);
			Matrix4x4 lightProjection = glm::ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, -15.0f, 15.0f);
			Matrix4x4 lightView = glm::lookAt(glm::normalize(lightDir), Vector3(0), UP);
			Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

			auto pShadowMapTexture = std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_SHADOWMAP_DEPTH));
			
			std::shared_ptr<ShaderProgram> pShaderProgram = nullptr;
			EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;

			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_OPAQUE)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_OPAQUE)), pCommandBuffer);

			UBTransformMatrices ubTransformMatrices;
			UBLightSpaceTransformMatrix ubLightSpaceTransformMatrix;
			UBCameraProperties ubCameraProperties;
			UBMaterialNumericalProperties ubMaterialNumericalProperties;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES));
			auto pLightSpaceTransformMatrixUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_LIGHTSPACE_TRANSFORM_MATRIX));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES));
			auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			ubLightSpaceTransformMatrix.lightSpaceMatrix = lightSpaceMatrix;
			auto pSubLightSpaceTransformMatrixUB = pLightSpaceTransformMatrixUB->AllocateSubBuffer(sizeof(UBLightSpaceTransformMatrix));
			pSubLightSpaceTransformMatrixUB->UpdateSubBufferData(&ubLightSpaceTransformMatrix);

			ubCameraProperties.cameraPosition = cameraPos;
			auto pSubCameraPropertiesUB = pCameraPropertiesUB->AllocateSubBuffer(sizeof(UBCameraProperties));
			pSubCameraPropertiesUB->UpdateSubBufferData(&ubCameraProperties);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				auto pSubTransformMatricesUB = pTransformMatricesUB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				unsigned int submeshCount = pMesh->GetSubmeshCount();
				auto subMeshes = pMesh->GetSubMeshes();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

				for (unsigned int i = 0; i < submeshCount; ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (pMaterial->IsTransparent())
					{
						continue;
					}

					if (lastUsedShaderProgramType != pMaterial->GetShaderProgramType())
					{
						pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(pMaterial->GetShaderProgramType(), HPForwardAsyncGraphRes::PASSNAME_OPAQUE), pCommandBuffer);
						pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
						lastUsedShaderProgramType = pMaterial->GetShaderProgramType();
					}
					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubCameraPropertiesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE), EDescriptorType::CombinedImageSampler, pShadowMapTexture);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX), EDescriptorType::SubUniformBuffer, pSubLightSpaceTransformMatrixUB);

					ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
					ubMaterialNumericalProperties.roughness = pMaterial->GetRoughness();
					ubMaterialNumericalProperties.anisotropy = pMaterial->GetAnisotropy();
					auto pSubMaterialNumericalPropertiesUB = pMaterialNumericalPropertiesUB->AllocateSubBuffer(sizeof(UBMaterialNumericalProperties));
					pSubMaterialNumericalPropertiesUB->UpdateSubBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubMaterialNumericalPropertiesUB);

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					auto pToneTexture = pMaterial->GetTexture(EMaterialTextureType::Tone);
					if (pToneTexture)
					{
						pToneTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TONE_TEXTURE), EDescriptorType::CombinedImageSampler, pToneTexture);
					}

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GNORMAL_TEXTURE), EDescriptorType::CombinedImageSampler,
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_NORMALONLY_NORMAL)));

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
				}

			}

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);			
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_OPAQUE, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_OPAQUE, pOpaquePass);
}

void HPForwardRenderer_Async::BuildGaussianBlurPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_BLUR_HORI, m_pBlurPassFrameBuffer_Horizontal);
	passInput.Add(HPForwardAsyncGraphRes::FB_BLUR_FIN, m_pBlurPassFrameBuffer_FinalColor);
	passInput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_BLUR_HORIZONTAL_COLOR, m_pBlurPassHorizontalColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::RPO_BLUR, m_pBlurRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);

	auto pGaussianBlurPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::GaussianBlur, HPForwardAsyncGraphRes::PASSNAME_BLUR), pCommandBuffer);

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES));

			// Horizontal subpass
			
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_BLUR)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_BLUR_HORI)), pCommandBuffer);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_COLOR)));

			ubControlVariables.bool_1 = 1;
			auto pSubControlVariablesUB_1 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_1->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_1);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Vertical subpass

			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_BLUR)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_BLUR_FIN)), pCommandBuffer);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BLUR_HORIZONTAL_COLOR)));

			ubControlVariables.bool_1 = 0;
			auto pSubControlVariablesUB_2 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_2->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_2);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);		
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_BLUR, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_BLUR, pGaussianBlurPass);
}

void HPForwardRenderer_Async::BuildLineDrawingPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_LINEDRAWING_CURV, m_pLineDrawingPassFrameBuffer_Curvature);
	passInput.Add(HPForwardAsyncGraphRes::FB_LINEDRAWING_RIDG, m_pLineDrawingPassFrameBuffer_RidgeSearch);
	passInput.Add(HPForwardAsyncGraphRes::FB_LINEDRAWING_BLUR_HORI, m_pLineDrawingPassFrameBuffer_Blur_Horizontal);
	passInput.Add(HPForwardAsyncGraphRes::FB_LINEDRAWING_BLUR_FIN, m_pLineDrawingPassFrameBuffer_Blur_FinalColor);
	passInput.Add(HPForwardAsyncGraphRes::FB_LINEDRAWING_FIN, m_pLineDrawingPassFrameBuffer_FinalBlend);
	passInput.Add(HPForwardAsyncGraphRes::TX_BLUR_FINAL_COLOR, m_pBlurPassFinalColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_COLOR, m_pOpaquePassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE, m_pLineDrawingMatrixTexture);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_CURVATURE, m_pLineDrawingPassCurvatureOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_BLURRED, m_pLineDrawingPassBlurredOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::RPO_LINEDRAWING, m_pLineDrawingRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);

	auto pLineDrawingPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			UBControlVariables ubControlVariables;
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES));

			// Curvature computation subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Curvature, HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_LINEDRAWING_CURV)), pCommandBuffer);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Curvature);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BLUR_FINAL_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SAMPLE_MATRIX_TEXTURE), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<RenderTexture>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_MATRIX_TEXTURE)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Ridge searching subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Color, HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_LINEDRAWING_RIDG)), pCommandBuffer);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Color);
			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_CURVATURE)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Gaussian blur subpasses

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::GaussianBlur, HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING), pCommandBuffer);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::GaussianBlur);

			// Horizontal subpass

			// Set render target to curvature color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_LINEDRAWING_BLUR_HORI)), pCommandBuffer);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR)));

			ubControlVariables.bool_1 = 1;
			auto pSubControlVariablesUB_1 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_1->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_1);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Vertical subpass

			// Set render target to blurred color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_LINEDRAWING_BLUR_FIN)), pCommandBuffer);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_CURVATURE)));

			ubControlVariables.bool_1 = 0;
			auto pSubControlVariablesUB_2 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_2->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_2);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Final blend subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::LineDrawing_Blend, HPForwardAsyncGraphRes::PASSNAME_LINEDRAWING), pCommandBuffer);

			// Set render target to color attachment
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_LINEDRAWING)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_LINEDRAWING_FIN)), pCommandBuffer);

			pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::LineDrawing_Blend);
			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_BLURRED)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_COLOR)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);			
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_LINEDRAWING, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_LINEDRAWING, pLineDrawingPass);
}

void HPForwardRenderer_Async::BuildTransparentPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_TRANSP, m_pTranspPassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(HPForwardAsyncGraphRes::RPO_TRANSP, m_pTranspRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES, m_pMaterialNumericalProperties_UB);

	passOutput.Add(HPForwardAsyncGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passOutput.Add(HPForwardAsyncGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);

	auto pTransparentPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);
			Matrix4x4 projectionMat = glm::perspective(pCameraComp->GetFOV(),
				gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowAspect(),
				pCameraComp->GetNearClip(), pCameraComp->GetFarClip());

			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBCameraProperties ubCameraProperties;
			UBMaterialNumericalProperties ubMaterialNumericalProperties;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES));
			auto pMaterialNumericalPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_MATERIAL_NUMERICAL_PROPERTIES));

			ubTransformMatrices.projectionMatrix = projectionMat;
			ubTransformMatrices.viewMatrix = viewMat;

			ubSystemVariables.timeInSec = Timer::Now();
			auto pSubSystemVariablesUB = pSystemVariablesUB->AllocateSubBuffer(sizeof(UBSystemVariables));
			pSubSystemVariablesUB->UpdateSubBufferData(&ubSystemVariables);

			ubCameraProperties.cameraPosition = cameraPos;
			auto pSubCameraPropertiesUB = pCameraPropertiesUB->AllocateSubBuffer(sizeof(UBCameraProperties));
			pSubCameraPropertiesUB->UpdateSubBufferData(&ubCameraProperties);

			std::shared_ptr<ShaderProgram> pShaderProgram = nullptr;
			EBuiltInShaderProgramType lastUsedShaderProgramType = EBuiltInShaderProgramType::NONE;

			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_TRANSP)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_TRANSP)), pCommandBuffer);

			for (auto& entity : *pContext->pDrawList)
			{
				auto pMaterialComp = std::static_pointer_cast<MaterialComponent>(entity->GetComponent(EComponentType::Material));
				auto pTransformComp = std::static_pointer_cast<TransformComponent>(entity->GetComponent(EComponentType::Transform));
				auto pMeshFilterComp = std::static_pointer_cast<MeshFilterComponent>(entity->GetComponent(EComponentType::MeshFilter));

				if (!pMaterialComp || pMaterialComp->GetMaterialCount() == 0 || !pTransformComp || !pMeshFilterComp)
				{
					continue;
				}

				auto pMesh = pMeshFilterComp->GetMesh();

				if (!pMesh)
				{
					continue;
				}

				ubTransformMatrices.modelMatrix = pTransformComp->GetModelMatrix();
				ubTransformMatrices.normalMatrix = pTransformComp->GetNormalMatrix();
				auto pSubTransformMatricesUB = pTransformMatricesUB->AllocateSubBuffer(sizeof(UBTransformMatrices));
				pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);

				auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

				unsigned int submeshCount = pMesh->GetSubmeshCount();
				auto subMeshes = pMesh->GetSubMeshes();

				pDevice->SetVertexBuffer(pMesh->GetVertexBuffer(), pCommandBuffer);

				for (unsigned int i = 0; i < submeshCount; ++i)
				{
					auto pMaterial = pMaterialComp->GetMaterialBySubmeshIndex(i);

					if (!pMaterial->IsTransparent()) // TODO: use a list to record these parts instead of checking one-by-one
					{
						continue;
					}

					if (lastUsedShaderProgramType != pMaterial->GetShaderProgramType())
					{
						pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(pMaterial->GetShaderProgramType(), HPForwardAsyncGraphRes::PASSNAME_TRANSPARENT), pCommandBuffer);
						pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(pMaterial->GetShaderProgramType());
						lastUsedShaderProgramType = pMaterial->GetShaderProgramType();
					}
					pShaderParamTable->Clear();

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubCameraPropertiesUB); // TODO: enable these disabled resources in shader program
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::SubUniformBuffer, pSubSystemVariablesUB);

					ubMaterialNumericalProperties.albedoColor = pMaterial->GetAlbedoColor();
					auto pSubMaterialNumericalPropertiesUB = pMaterialNumericalPropertiesUB->AllocateSubBuffer(sizeof(UBMaterialNumericalProperties));
					pSubMaterialNumericalPropertiesUB->UpdateSubBufferData(&ubMaterialNumericalProperties);
					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubMaterialNumericalPropertiesUB);

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_DEPTH)));

					pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
						std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR)));

					auto pAlbedoTexture = pMaterial->GetTexture(EMaterialTextureType::Albedo);
					if (pAlbedoTexture)
					{
						pAlbedoTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::ALBEDO_TEXTURE), EDescriptorType::CombinedImageSampler, pAlbedoTexture);
					}

					auto pNoiseTexture = pMaterial->GetTexture(EMaterialTextureType::Noise);
					if (pNoiseTexture)
					{
						pNoiseTexture->SetSampler(pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
						pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler, pNoiseTexture);
					}

					pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
					pDevice->DrawPrimitive(subMeshes->at(i).m_numIndices, subMeshes->at(i).m_baseIndex, subMeshes->at(i).m_baseVertex, pCommandBuffer);
				}
			}

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);	
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_TRANSP, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_TRANSP, pTransparentPass);
}

void HPForwardRenderer_Async::BuildOpaqueTranspBlendPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_BLEND, m_pBlendPassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR, m_pLineDrawingPassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_DEPTH, m_pOpaquePassDepthOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_TRANSP_COLOR, m_pTranspPassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_TRANSP_DEPTH, m_pTranspPassDepthOutput);
	passInput.Add(HPForwardAsyncGraphRes::RPO_BLEND, m_pBlendRenderPassObject);

	passOutput.Add(HPForwardAsyncGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);

	auto pBlendPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::DepthBased_ColorBlend_2, HPForwardAsyncGraphRes::PASSNAME_BLEND), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_BLEND)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_BLEND)), pCommandBuffer);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_DEPTH)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_LINEDRAWING_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::DEPTH_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_TRANSP_DEPTH)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_TRANSP_COLOR)));

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_COLORBLEND_D2, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_COLORBLEND_D2, pBlendPass);
}

void HPForwardRenderer_Async::BuildDepthOfFieldPass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_DOF_HORI, m_pDOFPassFrameBuffer_Horizontal);
	passInput.Add(HPForwardAsyncGraphRes::FB_PRESENT, m_pPresentFrameBuffers);
	passInput.Add(HPForwardAsyncGraphRes::TX_BLEND_COLOR, m_pBlendPassColorOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_NORMALONLY_POSITION, m_pNormalOnlyPassPositionOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_OPAQUE_SHADOW, m_pOpaquePassShadowOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_DOF_HORIZONTAL, m_pDOFPassHorizontalOutput);
	passInput.Add(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_1, m_pBrushMaskImageTexture_1);
	passInput.Add(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_2, m_pBrushMaskImageTexture_2);
	passInput.Add(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_1, m_pPencilMaskImageTexture_1);
	passInput.Add(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_2, m_pPencilMaskImageTexture_2);
	passInput.Add(HPForwardAsyncGraphRes::RPO_DOF, m_pDOFRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::RPO_PRESENT, m_pPresentRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES, m_pTransformMatrices_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES, m_pCameraPropertie_UB);
	passInput.Add(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES, m_pControlVariables_UB);

	auto pDOFPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pCameraTransform = std::static_pointer_cast<TransformComponent>(pContext->pCamera->GetComponent(EComponentType::Transform));
			auto pCameraComp = std::static_pointer_cast<CameraComponent>(pContext->pCamera->GetComponent(EComponentType::Camera));
			if (!pCameraComp || !pCameraTransform)
			{
				return;
			}
			Vector3 cameraPos = pCameraTransform->GetPosition();
			Matrix4x4 viewMat = glm::lookAt(cameraPos, cameraPos + pCameraTransform->GetForwardDirection(), UP);

			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::DOF);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			UBTransformMatrices ubTransformMatrices;
			UBSystemVariables ubSystemVariables;
			UBCameraProperties ubCameraProperties;
			UBControlVariables ubControlVariables;
			auto pTransformMatricesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_TRANSFORM_MATRICES));
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES));
			auto pCameraPropertiesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CAMERA_PROPERTIES));
			auto pControlVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_CONTROL_VARIABLES));

			ubTransformMatrices.viewMatrix = viewMat;
			auto pSubTransformMatricesUB = pTransformMatricesUB->AllocateSubBuffer(sizeof(UBTransformMatrices));
			pSubTransformMatricesUB->UpdateSubBufferData(&ubTransformMatrices);

			ubSystemVariables.timeInSec = Timer::Now();
			auto pSubSystemVariablesUB = pSystemVariablesUB->AllocateSubBuffer(sizeof(UBSystemVariables));
			pSubSystemVariablesUB->UpdateSubBufferData(&ubSystemVariables);

			ubCameraProperties.aperture = pCameraComp->GetAperture();
			ubCameraProperties.focalDistance = pCameraComp->GetFocalDistance();
			ubCameraProperties.imageDistance = pCameraComp->GetImageDistance();
			auto pSubCameraPropertiesUB = pCameraPropertiesUB->AllocateSubBuffer(sizeof(UBCameraProperties));
			pSubCameraPropertiesUB->UpdateSubBufferData(&ubCameraProperties);

			// Horizontal subpass

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::DOF, HPForwardAsyncGraphRes::PASSNAME_DOF), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_DOF)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_DOF_HORI)), pCommandBuffer);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubCameraPropertiesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BLEND_COLOR)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_NORMALONLY_POSITION)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::SubUniformBuffer, pSubSystemVariablesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_SHADOW)));

			ubControlVariables.bool_1 = 1;
			auto pSubControlVariablesUB_1 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_1->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_1);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			// Vertical subpass

			// Set to swapchain image output
			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::DOF, HPForwardAsyncGraphRes::PASSNAME_PRESENT), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_PRESENT)),
				std::static_pointer_cast<SwapchainFrameBuffers>(input.Get(HPForwardAsyncGraphRes::FB_PRESENT))->frameBuffers[pDevice->GetSwapchainPresentImageIndex()], pCommandBuffer);

			pShaderParamTable->Clear();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::TRANSFORM_MATRICES), EDescriptorType::SubUniformBuffer, pSubTransformMatricesUB);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CAMERA_PROPERTIES), EDescriptorType::SubUniformBuffer, pSubCameraPropertiesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_DOF_HORIZONTAL)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::GPOSITION_TEXTURE), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_NORMALONLY_POSITION)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::SubUniformBuffer, pSubSystemVariablesUB);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_BRUSH_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::MASK_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_1)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::NOISE_TEXTURE_2), EDescriptorType::CombinedImageSampler,
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_PENCIL_MASK_TEXTURE_2)));

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_2), EDescriptorType::CombinedImageSampler, 
				std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_OPAQUE_SHADOW)));

			ubControlVariables.bool_1 = 0;
			auto pSubControlVariablesUB_2 = pControlVariablesUB->AllocateSubBuffer(sizeof(UBControlVariables));
			pSubControlVariablesUB_2->UpdateSubBufferData(&ubControlVariables);
			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::CONTROL_VARIABLES), EDescriptorType::SubUniformBuffer, pSubControlVariablesUB_2);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			pDevice->EndCommandBuffer(pCommandBuffer);			
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_DOF, pCommandBuffer);
		},
		passInput,
		passOutput);

	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_DOF, pDOFPass);
}

void HPForwardRenderer_Async::BuildAsyncComputePass()
{
	RenderGraphResource passInput;
	RenderGraphResource passOutput;

	passInput.Add(HPForwardAsyncGraphRes::FB_ASYNC, m_pAsyncComputePassFrameBuffer);
	passInput.Add(HPForwardAsyncGraphRes::RPO_ASYNC, m_pAsyncComputeRenderPassObject);
	passInput.Add(HPForwardAsyncGraphRes::TX_ASYNC_INPUT, m_pAsyncComputeInputTexture);
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	passInput.Add(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB);
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	passInput.Add(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES, m_pSystemVariables_UB_IG);
#endif
	passInput.Add(HPForwardAsyncGraphRes::TB_ASYNC_OUTPUT, m_pAsyncComputeBuffer);

	passOutput.Add(HPForwardAsyncGraphRes::TX_ASYNC_OUTPUT, m_pAsyncComputeOutputTexture);
	passOutput.Add(HPForwardAsyncGraphRes::TB_ASYNC_OUTPUT, m_pAsyncComputeBuffer);

	auto pAsyncPass = std::make_shared<RenderNode>(
		[](const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext)
		{
			auto pDevice = pContext->pRenderer->GetDrawingDevice();
			auto pCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pCommandPool);

			auto pTransferBuffer = std::static_pointer_cast<DataTransferBuffer>(input.Get(HPForwardAsyncGraphRes::TB_ASYNC_OUTPUT));
			auto pInputTexture = std::static_pointer_cast<Texture2D>(input.Get(HPForwardAsyncGraphRes::TX_ASYNC_INPUT));
			pDevice->CopyDataTransferBufferToTexture2D(pTransferBuffer, pInputTexture, pCommandBuffer);
		
			auto pSystemVariablesUB = std::static_pointer_cast<UniformBuffer>(input.Get(HPForwardAsyncGraphRes::UB_SYSTEM_VARIABLES));

			UBSystemVariables ubSystemVariables;
			ubSystemVariables.timeInSec = Timer::Now();
			auto pSubSystemVariablesUB = pSystemVariablesUB->AllocateSubBuffer(sizeof(UBSystemVariables));
			pSubSystemVariablesUB->UpdateSubBufferData(&ubSystemVariables);

			pDevice->BindGraphicsPipeline(((HPForwardRenderer_Async*)(pContext->pRenderer))->GetGraphicsPipeline(EBuiltInShaderProgramType::AsyncCompute, HPForwardAsyncGraphRes::PASSNAME_ASYNC), pCommandBuffer);
			pDevice->BeginRenderPass(std::static_pointer_cast<RenderPassObject>(input.Get(HPForwardAsyncGraphRes::RPO_ASYNC)),
				std::static_pointer_cast<FrameBuffer>(input.Get(HPForwardAsyncGraphRes::FB_ASYNC)), pCommandBuffer);

			auto pShaderProgram = (pContext->pRenderer->GetDrawingSystem())->GetShaderProgramByType(EBuiltInShaderProgramType::AsyncCompute);
			auto pShaderParamTable = std::make_shared<ShaderParameterTable>();

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::COLOR_TEXTURE_1), EDescriptorType::CombinedImageSampler,
				pInputTexture);

			pShaderParamTable->AddEntry(pShaderProgram->GetParamBinding(ShaderParamNames::SYSTEM_VARIABLES), EDescriptorType::SubUniformBuffer, pSubSystemVariablesUB);

			pDevice->UpdateShaderParameter(pShaderProgram, pShaderParamTable, pCommandBuffer);
			pDevice->DrawFullScreenQuad(pCommandBuffer);

			pDevice->EndRenderPass(pCommandBuffer);

			auto pOutputTexture = std::static_pointer_cast<Texture2D>(output.Get(HPForwardAsyncGraphRes::TX_ASYNC_OUTPUT));
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
			auto pOutputFinishSemaphore = pDevice->RequestDrawingSemaphore(EGPUType::Discrete, ESemaphoreWaitStage::Transfer);
			pDevice->CommandSignalSemaphore(pCommandBuffer, pOutputFinishSemaphore);
			pDevice->EndCommandBuffer(pCommandBuffer);
			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteCommandRecordList(HPForwardAsyncGraphRes::NODE_ASYNC, pCommandBuffer);

			auto pTransferCommandBuffer = pDevice->RequestCommandBuffer(pCmdContext->pTransferCommandPool);
			auto pTransferSemaphore = pDevice->RequestDrawingSemaphore(EGPUType::Discrete, ESemaphoreWaitStage::ColorAttachmentOutput);
			pDevice->CommandWaitSemaphore(pTransferCommandBuffer, pOutputFinishSemaphore);
			pDevice->CommandSignalSemaphore(pTransferCommandBuffer, pTransferSemaphore);
			pDevice->CopyTexture2DToDataTransferBuffer(pOutputTexture, pTransferBuffer, pTransferCommandBuffer);
			pDevice->EndCommandBuffer(pTransferCommandBuffer);
			pDevice->ReturnExternalCommandBuffer(pTransferCommandBuffer);
			pDevice->FlushTransferCommands(false);

			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteAsyncComputeSemaphore(pTransferSemaphore);
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
			auto pTransferSemaphore = pDevice->RequestDrawingSemaphore(EGPUType::Integrated, ESemaphoreWaitStage::ColorAttachmentOutput);

			pDevice->CommandSignalSemaphore(pCommandBuffer, pTransferSemaphore);
			pDevice->CopyTexture2DToDataTransferBuffer(pOutputTexture, pTransferBuffer, pCommandBuffer);
			pDevice->EndCommandBuffer(pCommandBuffer);
			pDevice->ReturnExternalCommandBuffer(pCommandBuffer);
			pDevice->FlushCommands(false, false, (uint32_t)EGPUType::Integrated);

			((HPForwardRenderer_Async*)(pContext->pRenderer))->WriteAsyncComputeSemaphore(pTransferSemaphore);
#endif
		},
		passInput,
		passOutput);

#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	m_pRenderGraph->AddRenderNode(HPForwardAsyncGraphRes::NODE_ASYNC, pAsyncPass);
#endif
#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	m_pRenderGraph_IG->AddRenderNode(HPForwardAsyncGraphRes::NODE_ASYNC, pAsyncPass);
#endif
}

void HPForwardRenderer_Async::BuildRenderNodeDependencies()
{
	auto pShadowMapPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_SHADOWMAP);
	auto pNormalOnlyPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_NORMALONLY);
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	auto pAsyncPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_ASYNC);
#endif
	auto pOpaquePass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_OPAQUE);
	auto pGaussianBlurPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_BLUR);
	auto pLineDrawingPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_LINEDRAWING);
	auto pTransparentPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_TRANSP);
	auto pBlendPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_COLORBLEND_D2);
	auto pDOFPass = m_pRenderGraph->GetNodeByName(HPForwardAsyncGraphRes::NODE_DOF);

	pShadowMapPass->AddNextNode(pOpaquePass);
	pNormalOnlyPass->AddNextNode(pOpaquePass);
#if defined(ASYNC_COMPUTE_TEST_DISCRETE_GPU_CE)
	pAsyncPass->AddNextNode(pOpaquePass);
#endif
	pOpaquePass->AddNextNode(pGaussianBlurPass);
	pGaussianBlurPass->AddNextNode(pLineDrawingPass);
	pLineDrawingPass->AddNextNode(pTransparentPass);
	pTransparentPass->AddNextNode(pBlendPass);
	pBlendPass->AddNextNode(pDOFPass);

	m_pRenderGraph->BuildRenderNodePriorities();

	for (uint32_t i = 0; i < m_pRenderGraph->GetRenderNodeCount(); i++)
	{
		m_commandRecordReadyList.emplace(i, nullptr);
		m_commandRecordReadyListFlag.emplace(i, false);
	}

#if defined(ASYNC_COMPUTE_TEST_HETEROGENEOUS_GPUS_CE)
	m_pRenderGraph_IG->BuildRenderNodePriorities();
#endif
}

void HPForwardRenderer_Async::CreateLineDrawingMatrices()
{
	// Alert: current implementation does not support window scaling

	auto width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	auto height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

	// Storing the 6x9 sampling matrix
	// But a5 is not used, so we are only storing 5x9
	std::vector<float> linearResults;
	linearResults.resize(12 * 4, 0);
	m_pLineDrawingMatrixTexture = std::make_shared<RenderTexture>(12, 1); // Store the matrix in a 4-channel linear texture

	float xPixelSize = 1.0f / width;
	float yPixelSize = 1.0f / height;

	Eigen::MatrixXf H(6, 9);
	Eigen::MatrixXf X(9, 6);

	// 3x3 grid sample
	int rowIndex = 0;
	int lineWidth = 1;
	for (int m = -lineWidth; m < lineWidth + 1; m += lineWidth)
	{
		for (int n = -lineWidth; n < lineWidth + 1; n += lineWidth)
		{
			float xSample = m * xPixelSize;
			float ySample = n * yPixelSize;

			X(rowIndex, 0) = xSample * xSample;
			X(rowIndex, 1) = 2 * xSample * ySample;
			X(rowIndex, 2) = ySample * ySample;
			X(rowIndex, 3) = xSample;
			X(rowIndex, 4) = ySample;
			X(rowIndex, 5) = 1;

			rowIndex++;
		}
	}

	H = (X.transpose() * X).inverse() * X.transpose();

	// H is divided as
	// | 1 | 2 |   |
	// | 3 | 4 |   |
	// | 5 | 6 | 11|
	// | 7 | 8 |---|
	// | 9 | 10| 12|

	int linearIndex = 0;
	for (int p = 0; p < 5; ++p) // a5 is not used, so instead of 6 rows, 5 rows are taken
	{
		for (int q = 0; q < 8; ++q)
		{
			linearResults[linearIndex] = H(p, q);
			linearIndex++;
		}
	}

	// Store the 9th column
	for (int p = 0; p < 5; ++p)
	{
		linearResults[linearIndex] = H(p, 8);
		linearIndex++;
	}

	m_pLineDrawingMatrixTexture->FlushData(linearResults.data(), EDataType::Float32, ETextureFormat::RGBA32F);
	m_pLineDrawingMatrixTexture->SetSampler(m_pDevice->GetDefaultTextureSampler(EGPUType::Discrete));
}

void HPForwardRenderer_Async::ResetUniformBufferSubAllocation()
{
	m_pTransformMatrices_UB->ResetSubBufferAllocation();
	m_pLightSpaceTransformMatrix_UB->ResetSubBufferAllocation();
	m_pMaterialNumericalProperties_UB->ResetSubBufferAllocation();
	m_pCameraPropertie_UB->ResetSubBufferAllocation();
	m_pSystemVariables_UB->ResetSubBufferAllocation();
	m_pControlVariables_UB->ResetSubBufferAllocation();
}

void HPForwardRenderer_Async::ResetUniformBufferSubAllocation_IG()
{
	m_pSystemVariables_UB_IG->ResetSubBufferAllocation();
}

void HPForwardRenderer_Async::ApplyAsyncComputeResults()
{
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
	{
		std::unique_lock<std::mutex> lock(m_mainThreadMutex);
		m_mainThreadCv.wait(lock, [this]() { return m_finishedThreadCount == ASYNC_COMPUTE_THREAD_COUNT; });
		m_finishedThreadCount = 0;
	}
	memcpy(TransformComponent::Shared_PositionBuffer_Async + 1, Shared_PositionBuffer_Async, sizeof(Shared_PositionBuffer_Async)); // +1: skip camera transform
#else
#if defined(SIMULATE_DISCRETE_GPU_UNDER_PRESSURE_CE)
	assert(m_pPressureSimulationSemaphore != nullptr);
	m_pDevice->WaitSemaphore(m_pPressureSimulationSemaphore);
	m_pPressureSimulationSemaphore = nullptr;
#endif
	assert(m_pAsyncSemaphore != nullptr);
	m_pDevice->WaitSemaphore(m_pAsyncSemaphore);
	m_pDevice->CopyDataTransferBufferToHostDataLocation(m_pAsyncComputeBuffer, TransformComponent::Shared_PositionBuffer_Async + 1);
#endif
#endif
}

static float PseudoRand(Vector2 co)
{
	double discard;
	return std::modf(std::sin(glm::dot(co, Vector2(12.9898, 78.233))) * 43758.5453, &discard);
}

static Vector4 UpdatePosition(Vector4 origin, Vector2 seed)
{
	Vector4 result = origin;

	float coeff = 0.1f * PseudoRand(seed);

	Vector4 pressureVal = Vector4(0.0);
	for (int i = 0; i < 5000; i += 2)
	{
		pressureVal += 0.001f * Vector4(
			PseudoRand(Vector2(std::cos(PseudoRand(Vector2(i, origin.z))) * std::sin(PseudoRand(Vector2(i, origin.y))), seed.y)),
			PseudoRand(Vector2(std::sin(PseudoRand(Vector2(i, origin.x))) * std::cos(PseudoRand(Vector2(i, origin.z))), seed.y)),
			PseudoRand(Vector2(seed.y, std::cos(PseudoRand(Vector2(i, origin.y))) * std::sin(PseudoRand(Vector2(i, origin.x))))),
			0);
	}
	for (int i = 1; i < 5000; i += 2)
	{
		pressureVal -= 0.001f * Vector4(
			PseudoRand(Vector2(std::cos(PseudoRand(Vector2(i, origin.z))) * std::sin(PseudoRand(Vector2(i, origin.y))), seed.y)),
			PseudoRand(Vector2(std::sin(PseudoRand(Vector2(i, origin.x))) * std::cos(PseudoRand(Vector2(i, origin.z))), seed.y)),
			PseudoRand(Vector2(seed.y, std::cos(PseudoRand(Vector2(i, origin.y))) * std::sin(PseudoRand(Vector2(i, origin.x))))),
			0);
	}

	result += coeff * pressureVal;

	return result;
}

void HPForwardRenderer_Async::ExecuteAsycnCompute()
{
	std::shared_ptr<AsyncThreadTaskInfo> pTask = nullptr;

	while (m_isRunning)
	{
		{
			std::unique_lock<std::mutex> lock(m_asyncComputeMutex);
			m_asyncComputeCv.wait(lock, [this]() { return !m_asyncTaskQueue.Empty(); });
		}

		while (m_asyncTaskQueue.TryPop(pTask))
		{
#if defined(ASYNC_COMPUTE_TEST_CPU_CE)
			for (unsigned int i = pTask->beginIndex; i < pTask->endIndex; i++)
			{
				Shared_PositionBuffer_Async[i] = UpdatePosition(Shared_PositionBuffer_Async[i], Vector2(i, pTask->timeParam));
			}
#else
			std::cerr << "Incorrect async compute test mode.\n";
#endif
			m_finishedThreadCount++;
			if (m_finishedThreadCount == ASYNC_COMPUTE_THREAD_COUNT)
			{
				m_mainThreadCv.notify_all();
			}

			std::this_thread::yield();
		}
	}
}