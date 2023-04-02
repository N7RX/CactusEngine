#include "RenderGraph.h"
#include "Global.h"
#include "BaseEntity.h"
#include "BaseRenderer.h"
#include "GraphicsDevice.h"
#include "LogUtility.h"

namespace Engine
{
	void RenderGraphResource::Add(const char* name, RawResource* pResource)
	{
		m_renderResources.emplace(name, pResource);
	}

	RawResource* RenderGraphResource::Get(const char* name) const
	{
		if (m_renderResources.find(name) != m_renderResources.end())
		{
			return m_renderResources.at(name);
		}
		LOG_ERROR((std::string)"Couldn't find resource: " + name);
		return nullptr;
	}

	void RenderGraphResource::Swap(const char* name, RawResource* pResource)
	{
		if (m_renderResources.find(name) != m_renderResources.end())
		{
			m_renderResources.at(name) = pResource;
		}
		else
		{
			LOG_ERROR((std::string)"Couldn't find the resource to be swapped: " + name);
		}
	}

	RenderNode::RenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer)
		: m_pRenderer(pRenderer),
		m_pDevice(m_pRenderer->GetGraphicsDevice()),
		m_eGraphicsDeviceType(m_pDevice->GetGraphicsAPIType()),
		m_finishedExecution(false),
		m_pName(nullptr),
		m_pCmdContext(nullptr),
		m_graphResources(graphResources),
		m_frameIndex(0),
		m_pSwapchainImages(nullptr),
		m_outputToSwapchain(false),
		m_pRenderContext(nullptr),
		m_configuration()
	{
		
	}

	RenderNode::~RenderNode()
	{
		DestroyMutableResources();
		DestroyConstantResources();
	}

	void RenderNode::ConnectNext(RenderNode* pNode)
	{
		m_nextNodes.emplace_back(pNode);
		pNode->m_prevNodes.emplace_back(this);
	}

	void RenderNode::SetInputResource(const char* slot, const char* pResourceName)
	{
		DEBUG_ASSERT_CE(m_inputResourceNames.find(slot) != m_inputResourceNames.end());
		m_inputResourceNames.at(slot) = pResourceName;
	}

	void RenderNode::UseSwapchainImages(std::vector<Texture2D*>* pSwapchainImages)
	{
		DEBUG_ASSERT_CE(pSwapchainImages != nullptr);
		m_pSwapchainImages = pSwapchainImages;
		m_outputToSwapchain = true;
	}

	void RenderNode::Setup(const RenderNodeConfiguration& initInfo)
	{
		m_configuration = initInfo;

		CreateConstantResources(initInfo);
		CreateMutableResources(initInfo);
	}

	void RenderNode::ExecuteSequential()
	{
		for (auto& pNode : m_prevNodes)
		{
			if (!pNode->m_finishedExecution)
			{
				return;
			}
		}

		RenderPassFunction(m_graphResources[m_frameIndex], m_pRenderContext, m_pCmdContext);
		m_finishedExecution = true;

		for (auto& pNode : m_nextNodes)
		{
			pNode->ExecuteSequential();
		}
	}

	void RenderNode::ExecuteParallel()
	{
		RenderPassFunction(m_graphResources[m_frameIndex], m_pRenderContext, m_pCmdContext);
		m_finishedExecution = true;
	}

	void RenderNode::DestroyGraphicsPipelines()
	{
		for (auto pipeline : m_graphicsPipelines)
		{
			CE_DELETE(pipeline.second);
		}
	}

	RenderGraph::RenderGraph(GraphicsDevice* pDevice, uint32_t executionThreadCount)
		: m_pDevice(pDevice),
		m_isRunning(true),
		m_executionThreadCount(executionThreadCount)
	{
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType() == EGraphicsAPIType::Vulkan)
		{
			for (uint32_t i = 0; i < m_executionThreadCount; i++)
			{
				m_executionThreads.emplace_back(&RenderGraph::ExecuteRenderNodesParallel, this);
			}
		}
	}

	RenderGraph::~RenderGraph()
	{
		m_isRunning = false;

		for (uint32_t i = 0; i < m_executionThreadCount; i++)
		{
			m_executionThreads[i].join();
		}
	}

	void RenderGraph::AddRenderNode(const char* name, RenderNode* pNode)
	{
		m_nodes.emplace(name, pNode);
		m_nodes[name]->m_pName = name;
	}

	void RenderGraph::SetupRenderNodes()
	{
		RenderNodeConfiguration initInfo{};

		initInfo.width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		initInfo.height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();
		initInfo.framesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		initInfo.maxDrawCall = 256; // TODO: This should be updated according to scene complexity
		initInfo.colorFormat = ETextureFormat::RGBA8_SRGB;
		initInfo.swapSurfaceFormat = ETextureFormat::BGRA8_SRGB;
		initInfo.depthFormat = ETextureFormat::D32;
		initInfo.renderScale = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetRenderScale();

		for (auto& node : m_nodes)
		{
			node.second->Setup(initInfo);
		}
	}

	void RenderGraph::BuildRenderNodePriorities()
	{
		m_renderNodePriorities.clear();
		m_nodePriorityDependencies.clear();

		// Find all starting nodes
		std::queue<RenderNode*> startingNodes;
		for (auto& pNode : m_nodes)
		{
			pNode.second->m_finishedExecution = false;

			if (pNode.second->m_prevNodes.empty())
			{
				startingNodes.push(pNode.second);
			}
		}

		// Traverse graph
		std::vector<RenderNode*> traverseResult;
		while (!startingNodes.empty())
		{
			TraverseRenderNode(startingNodes.front(), traverseResult);
			startingNodes.pop();
		}

		// Reset flag
		for (auto& pNode : m_nodes)
		{
			pNode.second->m_finishedExecution = false;
		}

		// Assign priority by traverse sequence
		uint32_t assignedPriority = 0;
		for (uint32_t i = 0; i < traverseResult.size(); i++)
		{
			m_renderNodePriorities[traverseResult[i]->m_pName] = assignedPriority;
			assignedPriority++;
		}

		// Record priority dependencies
		for (uint32_t i = 0; i < traverseResult.size(); i++)
		{
			m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]] = std::vector<uint32_t>();
			for (auto pNode : traverseResult[i]->m_prevNodes)
			{
				m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]].emplace_back(m_renderNodePriorities[pNode->m_pName]);
			}
		}
	}

	void RenderGraph::BeginRenderPassesSequential(RenderContext* pContext, uint32_t frameIndex)
	{
		static CommandContext* pEmptyCmdContext;
		if (!pEmptyCmdContext)
		{
			CE_NEW(pEmptyCmdContext, CommandContext);
		}

		for (auto& pNode : m_nodes)
		{
			pNode.second->m_pRenderContext = pContext;
			pNode.second->m_pCmdContext = pEmptyCmdContext;
			pNode.second->m_finishedExecution = false;
			pNode.second->m_frameIndex = frameIndex;

			if (pNode.second->m_prevNodes.empty())
			{
				m_startingNodes.push(pNode.second);
			}
		}

		while (!m_startingNodes.empty())
		{
			m_startingNodes.front()->ExecuteSequential();
			m_startingNodes.pop();
		}
	}

	void RenderGraph::BeginRenderPassesParallel(RenderContext* pContext, uint32_t frameIndex)
	{
		for (auto& pNode : m_nodes)
		{
			pNode.second->m_pRenderContext = pContext;
			pNode.second->m_finishedExecution = false;
			pNode.second->m_frameIndex = frameIndex;

			if (pNode.second->m_prevNodes.empty())
			{
				m_startingNodes.push(pNode.second);
			}
		}

		{
			std::lock_guard<std::mutex> guard(m_nodeExecutionMutex);

			while (!m_startingNodes.empty())
			{
				EnqueueRenderNode(m_startingNodes.front());
				m_startingNodes.pop();
			}

			for (auto& pNode : m_nodes)
			{
				pNode.second->m_finishedExecution = false;
			}
		}

		m_nodeExecutionCv.notify_all();
	}

	RenderNode* RenderGraph::GetNodeByName(const char* name) const
	{
		if (m_nodes.find(name) != m_nodes.end())
		{
			return m_nodes.at(name);
		}
		LOG_ERROR((std::string)"Coundn't find node: " + name);
		return nullptr;
	}

	uint32_t RenderGraph::GetRenderNodeCount() const
	{
		return m_nodes.size();
	}

	void RenderGraph::UpdateResolution(uint32_t width, uint32_t height)
	{
		for (auto& pNode : m_nodes)
		{
			pNode.second->UpdateResolution(width, height);
		}
	}

	void RenderGraph::UpdateFramesInFlight(uint32_t framesInFlight)
	{
		for (auto& pNode : m_nodes)
		{
			pNode.second->UpdateFramesInFlight(framesInFlight);
		}
	}

	void RenderGraph::ExecuteRenderNodesParallel()
	{
		CommandContext* pCmdContext;
		CE_NEW(pCmdContext, CommandContext);
		pCmdContext->pCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Graphics);
		pCmdContext->pTransferCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Transfer);

		RenderNode* pNode = nullptr;
		while (m_isRunning)
		{
			{
				std::unique_lock<std::mutex> lock(m_nodeExecutionMutex);
				m_nodeExecutionCv.wait(lock, [this]() { return !m_executionNodeQueue.Empty(); });
			}

			while (m_executionNodeQueue.TryPop(pNode))
			{
				pNode->m_pCmdContext = pCmdContext;
				pNode->ExecuteParallel();

				std::this_thread::yield();
			}
		}
	}

	void RenderGraph::EnqueueRenderNode(RenderNode* pNode)
	{
		// Enqueue render nodes by dependency sequence
		for (auto& pPrevNode : pNode->m_prevNodes)
		{
			if (!pPrevNode->m_finishedExecution) // m_finishedExecution is being used as traversal flag here
			{
				return;
			}
		}

		m_executionNodeQueue.Push(pNode);
		pNode->m_finishedExecution = true;

		for (auto& pNextNode : pNode->m_nextNodes)
		{
			EnqueueRenderNode(pNextNode);
		}
	}

	void RenderGraph::TraverseRenderNode(RenderNode* pNode, std::vector<RenderNode*>& output)
	{
		// Record render nodes by dependency sequence
		for (auto& pPrevNode : pNode->m_prevNodes)
		{
			if (!pPrevNode->m_finishedExecution) // m_finishedExecution is being used as traversal flag here
			{
				return;
			}
		}

		output.emplace_back(pNode);
		pNode->m_finishedExecution = true;

		for (auto& pNextNode : pNode->m_nextNodes)
		{
			TraverseRenderNode(pNextNode, output);
		}
	}
}