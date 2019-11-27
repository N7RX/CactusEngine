#include "RenderGraph.h"
#include <assert.h>

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
	return nullptr;
}

RenderNode::RenderNode(const std::shared_ptr<RenderGraph> pRenderGraph, void(*pRenderPassFunc)(const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext), const RenderGraphResource& input, const RenderGraphResource& output)
	: m_pRenderGraph(pRenderGraph), m_pRenderPassFunc(pRenderPassFunc), m_input(input), m_output(output)
{
}

void RenderNode::AddNextNode(std::shared_ptr<RenderNode> pNode)
{
	m_nextNodes.emplace_back(pNode);
	pNode->m_prevNodes.emplace_back(this);
}

void RenderNode::Execute()
{
	assert(m_pRenderPassFunc != nullptr);
	m_pRenderPassFunc(m_input, m_output, m_pContext);

	for (auto& pNode : m_nextNodes) // Alert: we might run into duplicate execution
	{
		pNode->Execute();
	}
}

void RenderGraph::AddRenderNode(const char* name, std::shared_ptr<RenderNode> pNode)
{
	m_nodes.emplace(name, pNode);
}

void RenderGraph::BeginRenderPasses(const std::shared_ptr<RenderContext> pContext)
{
	for (auto& pNode : m_nodes)
	{
		pNode.second->m_pContext = pContext;
		if (pNode.second->m_prevNodes.empty())
		{
			m_startingNodes.push(pNode.second);
		}
	}

	// Alert: current implementation doesn't have optimized scheduling
	while (!m_startingNodes.empty())
	{
		m_startingNodes.front()->Execute();
		m_startingNodes.pop();
	}
}

std::shared_ptr<RenderNode> RenderGraph::GetNodeByName(const char* name) const
{
	if (m_nodes.find(name) != m_nodes.end())
	{
		return m_nodes.at(name);
	}
	return nullptr;
}