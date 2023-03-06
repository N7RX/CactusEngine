#include "StandardRenderer.h"
#include "RenderingSystem.h"
#include "AllComponents.h"
#include "AllRenderNodes.h"
#include "Timer.h"

namespace Engine
{
	StandardRenderer::StandardRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem)
		: BaseRenderer(ERendererType::Standard, pDevice, pSystem),
		m_newCommandRecorded(false)
	{
		CE_NEW(m_pGraphResources, RenderGraphResource);
	}

	void StandardRenderer::BuildRenderGraph()
	{
		CE_NEW(m_pRenderGraph, RenderGraph, m_pDevice, 4);

		// Create required nodes

		ShadowMapRenderNode* pShadowMapNode;
		CE_NEW(pShadowMapNode, ShadowMapRenderNode, m_pGraphResources, this);
		GBufferRenderNode* pGBufferNode;
		CE_NEW(pGBufferNode, GBufferRenderNode, m_pGraphResources, this);
		OpaqueContentRenderNode* pOpaqueNode;
		CE_NEW(pOpaqueNode, OpaqueContentRenderNode, m_pGraphResources, this);
		DeferredLightingRenderNode* pDeferredLightingNode;
		CE_NEW(pDeferredLightingNode, DeferredLightingRenderNode, m_pGraphResources, this);
		BlurRenderNode* pBlurNode;
		CE_NEW(pBlurNode, BlurRenderNode, m_pGraphResources, this);
		LineDrawingRenderNode* pLineDrawingNode;
		CE_NEW(pLineDrawingNode, LineDrawingRenderNode, m_pGraphResources, this);
		TransparentContentRenderNode* pTransparencyNode;
		CE_NEW(pTransparencyNode, TransparentContentRenderNode, m_pGraphResources, this);
		TransparencyBlendRenderNode* pBlendNode;
		CE_NEW(pBlendNode, TransparencyBlendRenderNode, m_pGraphResources, this);
		DepthOfFieldRenderNode* pDOFNode;
		CE_NEW(pDOFNode, DepthOfFieldRenderNode, m_pGraphResources, this);

		m_pRenderGraph->AddRenderNode("ShadowMapNode", pShadowMapNode);
		m_pRenderGraph->AddRenderNode("GBufferNode", pGBufferNode);
		m_pRenderGraph->AddRenderNode("OpaqueNode", pOpaqueNode);
		m_pRenderGraph->AddRenderNode("DeferredLightingNode", pDeferredLightingNode);
		m_pRenderGraph->AddRenderNode("BlurNode", pBlurNode);
		m_pRenderGraph->AddRenderNode("LineDrawingNode", pLineDrawingNode);
		m_pRenderGraph->AddRenderNode("TransparencyNode", pTransparencyNode);
		m_pRenderGraph->AddRenderNode("BlendNode", pBlendNode);
		m_pRenderGraph->AddRenderNode("DOFNode", pDOFNode);

		pShadowMapNode->ConnectNext(pOpaqueNode);
		pGBufferNode->ConnectNext(pOpaqueNode);
		pOpaqueNode->ConnectNext(pDeferredLightingNode);
		pDeferredLightingNode->ConnectNext(pBlurNode);
		pBlurNode->ConnectNext(pLineDrawingNode);
		pLineDrawingNode->ConnectNext(pTransparencyNode);
		pTransparencyNode->ConnectNext(pBlendNode);
		pBlendNode->ConnectNext(pDOFNode);

		// Define resource dependencies

		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_SHADOW_MAP, ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE);
		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_GBUFFER_NORMAL, GBufferRenderNode::OUTPUT_NORMAL_GBUFFER);

		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_COLOR, OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_NORMAL, GBufferRenderNode::OUTPUT_NORMAL_GBUFFER);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_POSITION, GBufferRenderNode::OUTPUT_POSITION_GBUFFER);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_DEPTH_TEXTURE, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlurNode->SetInputResource(BlurRenderNode::INPUT_COLOR_TEXTURE, OpaqueContentRenderNode::OUTPUT_LINE_SPACE_TEXTURE);

		pLineDrawingNode->SetInputResource(LineDrawingRenderNode::INPUT_COLOR_TEXTURE, DeferredLightingRenderNode::OUTPUT_COLOR_TEXTURE);
		pLineDrawingNode->SetInputResource(LineDrawingRenderNode::INPUT_LINE_SPACE_TEXTURE, BlurRenderNode::OUTPUT_COLOR_TEXTURE);

		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_COLOR_TEXTURE, LineDrawingRenderNode::OUTPUT_COLOR_TEXTURE);
		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_BACKGROUND_DEPTH, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_COLOR_TEXTURE, LineDrawingRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_DEPTH_TEXTURE, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_COLOR_TEXTURE, TransparentContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_DEPTH_TEXTURE, TransparentContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pDOFNode->SetInputResource(DepthOfFieldRenderNode::INPUT_COLOR_TEXTURE, TransparencyBlendRenderNode::OUTPUT_COLOR_TEXTURE);
		pDOFNode->SetInputResource(DepthOfFieldRenderNode::INPUT_GBUFFER_POSITION, GBufferRenderNode::OUTPUT_POSITION_GBUFFER);
		pDOFNode->SetInputResource(DepthOfFieldRenderNode::INPUT_SHADOW_MARK_TEXTURE, OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE);

		// Initialize render graph

		m_pRenderGraph->SetupRenderNodes();
		m_pRenderGraph->BuildRenderNodePriorities();

		for (uint32_t i = 0; i < m_pRenderGraph->GetRenderNodeCount(); i++)
		{
			m_commandRecordReadyList.emplace(i, nullptr);
			m_commandRecordReadyListFlag.emplace(i, false);
		}
	}

	void StandardRenderer::Draw(const std::vector<BaseEntity*>& drawList, BaseEntity* pCamera)
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

			m_pRenderGraph->BeginRenderPassesParallel(&currentContext);

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
						[](const std::pair<uint32_t, GraphicsCommandBuffer*>& lhs, std::pair<uint32_t, GraphicsCommandBuffer*>& rhs)
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
		else // OpenGL
		{
			m_pRenderGraph->BeginRenderPassesSequential(&currentContext);
		}
	}

	void StandardRenderer::WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer)
	{
		{
			std::lock_guard<std::mutex> guard(m_commandRecordListWriteMutex);

			m_commandRecordReadyList[m_pRenderGraph->m_renderNodePriorities[pNodeName]] = pCommandBuffer;
			m_writtenCommandPriorities.push(m_pRenderGraph->m_renderNodePriorities[pNodeName]);
			m_newCommandRecorded = true;
		}

		m_commandRecordListCv.notify_all();
	}
}