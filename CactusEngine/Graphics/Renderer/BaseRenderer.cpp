#include "BaseRenderer.h"
#include "GraphicsDevice.h"
#include "RenderGraph.h"

namespace Engine
{
	BaseRenderer::BaseRenderer(ERendererType type, GraphicsDevice* pDevice, RenderingSystem* pSystem)
		: m_rendererType(type),
		m_renderPriority(0),
		m_pRenderGraph(nullptr),
		m_pDevice(pDevice),
		m_pSystem(pSystem),
		m_eGraphicsDeviceType(pDevice->GetGraphicsAPIType()),
		m_newCommandRecorded(false)
	{
		uint32_t maxFramesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		m_graphResources.resize(maxFramesInFlight);
		for (uint32_t i = 0; i < maxFramesInFlight; ++i)
		{
			CE_NEW(m_graphResources[i], RenderGraphResource);
		}
	}

	void BaseRenderer::Draw(const std::vector<BaseEntity*>& drawList, BaseEntity* pCamera, uint32_t frameIndex)
	{
		if (!pCamera)
		{
			return;
		}

		RenderContext currentContext{};
		currentContext.pCamera = pCamera;
		currentContext.pDrawList = &drawList;

		if (m_eGraphicsDeviceType == EGraphicsAPIType::Vulkan)
		{
			for (auto& item : m_commandRecordReadyList)
			{
				item.second = nullptr;
				m_commandRecordReadyListFlag[item.first] = false;
			}

			m_pRenderGraph->BeginRenderPassesParallel(&currentContext, frameIndex);

			// Submit async recorded command buffers by correct sequence

			uint32_t finishedNodeCount = 0;

			while (finishedNodeCount < m_pRenderGraph->GetRenderNodeCount())
			{
				std::vector<std::pair<uint32_t, GraphicsCommandBuffer*>> buffersToReturn;

				{
					std::unique_lock<std::mutex> lock(m_commandRecordListWriteMutex);
					m_commandRecordListCv.wait(lock, [this]() { return m_newCommandRecorded; });
					m_newCommandRecorded = false;

					std::vector<uint32_t> sortedQueueContents;
					std::queue<uint32_t>  continueWaitQueue; // Command buffers that shouldn't be submitted as dependencies have not finished

					// Eliminate dependency jump
					while (!m_writtenCommandPriorities.empty())
					{
						sortedQueueContents.emplace_back(m_writtenCommandPriorities.front());
						m_writtenCommandPriorities.pop();
					}
					std::sort(sortedQueueContents.begin(), sortedQueueContents.end());

					for (uint32_t i = 0; i < sortedQueueContents.size(); i++)
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
						[](const std::pair<uint32_t, GraphicsCommandBuffer*>& lhs, std::pair<uint32_t, GraphicsCommandBuffer*>& rhs)
						{
							return lhs.first < rhs.first;
						});

					for (uint32_t i = 0; i < buffersToReturn.size(); i++)
					{
						m_pDevice->ReturnExternalCommandBuffer(buffersToReturn[i].second);
					}

					finishedNodeCount += buffersToReturn.size();
					m_pDevice->FlushCommands(false, false);
				}
			}
		}
		else // OpenGL
		{
			m_pRenderGraph->BeginRenderPassesSequential(&currentContext, frameIndex);
		}
	}

	void BaseRenderer::WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer)
	{
		if (!pCommandBuffer)
		{
			DEBUG_ASSERT_CE(m_eGraphicsDeviceType == EGraphicsAPIType::OpenGL);
			return;
		}

		{
			std::lock_guard<std::mutex> guard(m_commandRecordListWriteMutex);

			m_commandRecordReadyList[m_pRenderGraph->m_renderNodePriorities[pNodeName]] = pCommandBuffer;
			m_writtenCommandPriorities.push(m_pRenderGraph->m_renderNodePriorities[pNodeName]);
			m_newCommandRecorded = true;
		}

		m_commandRecordListCv.notify_all();
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

	GraphicsDevice* BaseRenderer::GetGraphicsDevice() const
	{
		return m_pDevice;
	}

	RenderingSystem* BaseRenderer::GetRenderingSystem() const
	{
		return m_pSystem;
	}

	void BaseRenderer::ObtainSwapchainImages()
	{
		m_swapchainImages.clear();
		m_pDevice->GetSwapchainImages(m_swapchainImages);
	}
}