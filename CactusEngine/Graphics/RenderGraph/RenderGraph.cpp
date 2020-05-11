#include "RenderGraph.h"
#include "DrawingDevice.h"
#include "BaseRenderer.h"
#include <assert.h>
#include <iostream>

using namespace Engine;

void RenderGraphResource::Add(const char* name, std::shared_ptr<RawResource> pResource)
{
	m_renderResources.emplace(name, pResource);
}

std::shared_ptr<RawResource> RenderGraphResource::Get(const char* name) const
{
	if (m_renderResources.find(name) != m_renderResources.end())
	{
		return m_renderResources.at(name);
	}
	std::cerr << "Couldn't find resource: " << name << std::endl;
	return nullptr;
}

void RenderGraphResource::Swap(const char* name, std::shared_ptr<RawResource> pResource)
{
	if (m_renderResources.find(name) != m_renderResources.end())
	{
		m_renderResources.at(name) = pResource;
	}
	else
	{
		std::cerr << "Couldn't find the resource to be swapped: " << name << std::endl;
	}
}

RenderNode::RenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer)
	: m_pRenderer(pRenderer), m_pGraphResources(pGraphResources), m_finishedExecution(false), m_pName(nullptr)
{
	assert(m_pGraphResources != nullptr);
	m_pDevice = m_pRenderer->GetDrawingDevice();
	m_eGraphicsDeviceType = m_pDevice->GetDeviceType();
}

void RenderNode::ConnectNext(std::shared_ptr<RenderNode> pNode)
{
	m_nextNodes.emplace_back(pNode);
	pNode->m_prevNodes.emplace_back(this);
}

void RenderNode::SetInputResource(const char* slot, const char* pResourceName)
{
	assert(m_inputResourceNames.find(slot) != m_inputResourceNames.end());
	m_inputResourceNames.at(slot) = pResourceName;
}

void RenderNode::Setup()
{
	SetupFunction(m_pGraphResources);
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

	RenderPassFunction(m_pGraphResources, m_pRenderContext, m_pCmdContext);
	m_finishedExecution = true;

	for (auto& pNode : m_nextNodes)
	{
		pNode->ExecuteSequential();
	}
}

void RenderNode::ExecuteParallel()
{
	RenderPassFunction(m_pGraphResources, m_pRenderContext, m_pCmdContext);
	m_finishedExecution = true;
}

RenderGraph::RenderGraph(const std::shared_ptr<DrawingDevice> pDevice, uint32_t executionThreadCount, EGPUType deviceType)
	: m_pDevice(pDevice), m_isRunning(true), m_executionThreadCount(executionThreadCount), m_deviceType(deviceType)
{
	if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType() == EGraphicsDeviceType::Vulkan)
	{
		for (unsigned int i = 0; i < m_executionThreadCount; i++)
		{
			m_executionThreads.emplace_back(&RenderGraph::ExecuteRenderNodeParallel, this);
		}
	}
}

RenderGraph::~RenderGraph()
{
	m_isRunning = false;

	for (unsigned int i = 0; i < m_executionThreadCount; i++)
	{
		m_executionThreads[i].join();
	}
}

void RenderGraph::AddRenderNode(const char* name, std::shared_ptr<RenderNode> pNode)
{
	m_nodes.emplace(name, pNode);
	m_nodes[name]->m_pName = name;
}

void RenderGraph::SetupRenderNodes()
{
	for (auto& node : m_nodes)
	{
		node.second->Setup();
	}
}

void RenderGraph::BuildRenderNodePriorities()
{
	m_renderNodePriorities.clear();
	m_nodePriorityDependencies.clear();

	// Find all starting nodes
	std::queue<std::shared_ptr<RenderNode>> startingNodes;
	for (auto& pNode : m_nodes)
	{
		pNode.second->m_finishedExecution = false;

		if (pNode.second->m_prevNodes.empty())
		{
			startingNodes.push(pNode.second);
		}
	}

	// Traverse graph
	std::vector<std::shared_ptr<RenderNode>> traverseResult;
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
	for (unsigned int i = 0; i < traverseResult.size(); i++)
	{
		m_renderNodePriorities[traverseResult[i]->m_pName] = assignedPriority;
		assignedPriority++;
	}

	// Record priority dependencies
	for (unsigned int i = 0; i < traverseResult.size(); i++)
	{
		m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]] = std::vector<uint32_t>();
		for (auto pNode : traverseResult[i]->m_prevNodes)
		{
			m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]].emplace_back(m_renderNodePriorities[pNode->m_pName]);
		}
	}
}

void RenderGraph::BeginRenderPassesSequential(const std::shared_ptr<RenderContext> pContext)
{
	static std::shared_ptr<CommandContext> pEmptyCmdContext = std::make_shared<CommandContext>();

	for (auto& pNode : m_nodes)
	{
		pNode.second->m_pRenderContext = pContext;
		pNode.second->m_pCmdContext = pEmptyCmdContext;
		pNode.second->m_finishedExecution = false;

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

void RenderGraph::BeginRenderPassesParallel(const std::shared_ptr<RenderContext> pContext)
{
	for (auto& pNode : m_nodes)
	{
		pNode.second->m_pRenderContext = pContext;
		pNode.second->m_finishedExecution = false;

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

std::shared_ptr<RenderNode> RenderGraph::GetNodeByName(const char* name) const
{
	if (m_nodes.find(name) != m_nodes.end())
	{
		return m_nodes.at(name);
	}
	std::cerr << "Coundn't find node: " << name << std::endl;
	return nullptr;
}

uint32_t RenderGraph::GetRenderNodeCount() const
{
	return m_nodes.size();
}

void RenderGraph::ExecuteRenderNodeParallel()
{
	auto pCmdContext = std::make_shared<CommandContext>();
	pCmdContext->pCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Graphics);
	pCmdContext->pTransferCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Transfer);

	std::shared_ptr<RenderNode> pNode = nullptr;
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

void RenderGraph::EnqueueRenderNode(const std::shared_ptr<RenderNode> pNode)
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

void RenderGraph::TraverseRenderNode(const std::shared_ptr<RenderNode> pNode, std::vector<std::shared_ptr<RenderNode>>& output)
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