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
	pNode->m_prevNodes.push_back(std::static_pointer_cast<RenderNode>(shared_from_this()));
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

void RenderGraph::AddRenderNode(std::shared_ptr<RenderNode> pNode)
{
	m_nodes.emplace_back(pNode);
}

void RenderGraph::BeginRenderPasses(const std::shared_ptr<RenderContext> pContext)
{
	for (auto& pNode : m_nodes)
	{
		pNode->m_pContext = pContext;
		if (pNode->m_prevNodes.empty())
		{
			m_startingNodes.push(pNode);
		}
	}

	// Alert: current implementation doesn't have optimized scheduling
	while (!m_startingNodes.empty())
	{
		m_startingNodes.front()->Execute();
		m_startingNodes.pop();
	}
}