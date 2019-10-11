#include "RenderGraph.h"
#include <assert.h>

using namespace Engine;

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

	for (auto& pNode : m_nextNodes)
	{
		pNode->Execute();
	}
}

void RenderGraph::AddRenderNode(ERenderNodeType type, std::shared_ptr<RenderNode> pNode)
{
	m_nodes.emplace(type, pNode);
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

std::shared_ptr<RenderNode> RenderGraph::GetNodeByType(ERenderNodeType type) const
{
	if (m_nodes.find(type) != m_nodes.end())
	{
		return m_nodes.at(type);
	}
	return nullptr;
}