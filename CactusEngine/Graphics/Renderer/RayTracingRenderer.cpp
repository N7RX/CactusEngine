#include "RayTracingRenderer.h"
#include "DrawingSystem.h"
#include "AllComponents.h"
#include "Timer.h"
#include "AllRenderNodes.h"

#include <iostream>

using namespace Engine;

RayTracingRenderer::RayTracingRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(ERendererType::RayTracing, pDevice, pSystem), m_newCommandRecorded(false)
{
	m_pGraphResources = std::make_shared<RenderGraphResource>();
}

void RayTracingRenderer::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>(m_pDevice, 4);

	// Create required nodes


	// Define resource dependencies


	// Initialize render graph

	m_pRenderGraph->SetupRenderNodes();
	m_pRenderGraph->BuildRenderNodePriorities();

	for (uint32_t i = 0; i < m_pRenderGraph->GetRenderNodeCount(); i++)
	{
		m_commandRecordReadyList.emplace(i, nullptr);
		m_commandRecordReadyListFlag.emplace(i, false);
	}
}

void RayTracingRenderer::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pContext = std::make_shared<RenderContext>();
	pContext->pCamera = pCamera;
	pContext->pDrawList = &drawList;

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
			m_pDevice->FlushCommands(false, false);
		}
	}
}

void RayTracingRenderer::WriteCommandRecordList(const char* pNodeName, const std::shared_ptr<DrawingCommandBuffer>& pCommandBuffer)
{
	{
		std::lock_guard<std::mutex> guard(m_commandRecordListWriteMutex);

		m_commandRecordReadyList[m_pRenderGraph->m_renderNodePriorities[pNodeName]] = pCommandBuffer;		
		m_writtenCommandPriorities.push(m_pRenderGraph->m_renderNodePriorities[pNodeName]);
		m_newCommandRecorded = true;
	}

	m_commandRecordListCv.notify_all();
}