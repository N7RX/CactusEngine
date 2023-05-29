#include "BaseRenderer.h"
#include "GraphicsDevice.h"
#include "RenderGraph.h"
#include "RenderingSystem.h"

namespace Engine
{
	BaseRenderer::BaseRenderer(ERendererType type, GraphicsDevice* pDevice, RenderingSystem* pSystem)
		: m_rendererType(type),
		m_renderPriority(0),
		m_pRenderGraph(nullptr),
		m_pDevice(pDevice),
		m_pSystem(pSystem),
		m_eGraphicsDeviceType(pDevice->GetGraphicsAPIType()),
		m_finishedNodeCount(0),
		m_commandRecordFinished(false)
	{
		uint32_t maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		m_graphResources.resize(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			CE_NEW(m_graphResources[i], RenderGraphResource);
		}

		m_pDevice->CreateUniformBufferManager(m_pBufferManager);
	}

	void BaseRenderer::Draw(const RenderContext& renderContext, uint32_t frameIndex)
	{
		if (!renderContext.pCamera)
		{
			return;
		}

		for (auto& item : m_commandRecordReadyList)
		{
			item = nullptr;
		}
		m_finishedNodeCount = 0;
		m_commandRecordFinished = false;

		m_pBufferManager->SetCurrentFrameIndex(frameIndex);
		m_pBufferManager->ResetBufferAllocation();

		m_pRenderGraph->BeginRenderPassesParallel(renderContext, frameIndex);

		{
			std::unique_lock<std::mutex> lock(m_commandRecordListWriteMutex);
			m_commandRecordListCv.wait(lock, [this]() { return m_commandRecordFinished; });
		}

		// Submit async recorded command buffers by correct sequence
		// One batch per frame
		m_pDevice->ReturnMultipleExternalCommandBuffer(m_commandRecordReadyList);
		m_pDevice->FlushCommands(false, false);
	}

	void BaseRenderer::WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer)
	{
		DEBUG_ASSERT_CE(pCommandBuffer != nullptr);
		{
			std::lock_guard<std::mutex> guard(m_commandRecordListWriteMutex);

			m_commandRecordReadyList[m_pRenderGraph->m_renderNodePriorities[pNodeName]] = pCommandBuffer;
			++m_finishedNodeCount;

			DEBUG_ASSERT_CE(m_finishedNodeCount <= m_pRenderGraph->GetRenderNodeCount()); // Check for overwriting
			if (m_finishedNodeCount == m_pRenderGraph->GetRenderNodeCount())
			{
				m_commandRecordFinished = true;
				m_commandRecordListCv.notify_one();
			}
		}
	}

	ERendererType BaseRenderer::GetRendererType() const
	{
		return m_rendererType;
	}

	void BaseRenderer::SetRendererPriority(uint32_t priority)
	{
		m_renderPriority = priority;
	}

	uint32_t BaseRenderer::GetRendererPriority() const
	{
		return m_renderPriority;
	}

	RenderGraph* BaseRenderer::GetRenderGraph() const
	{
		return m_pRenderGraph;
	}

	GraphicsDevice* BaseRenderer::GetGraphicsDevice() const
	{
		return m_pDevice;
	}

	RenderingSystem* BaseRenderer::GetRenderingSystem() const
	{
		return m_pSystem;
	}

	UniformBufferManager* BaseRenderer::GetBufferManager() const
	{
		return m_pBufferManager;
	}

	void BaseRenderer::UpdateResolution(uint32_t width, uint32_t height)
	{
		ObtainSwapchainImages();
		m_pRenderGraph->UpdateResolution(width, height);
	}

	void BaseRenderer::ObtainSwapchainImages()
	{
		m_swapchainImages.clear();
		m_pDevice->GetSwapchainImages(m_swapchainImages);
	}
}