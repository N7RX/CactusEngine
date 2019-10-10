#pragma once
#include "DrawingResources.h"
#include "IEntity.h"
#include "NoCopy.h"
#include <queue>

namespace Engine
{
	struct RenderGraphResource
	{
		std::vector<std::shared_ptr<RawResource>> renderResources;
	};

	class BaseRenderer;
	struct RenderContext
	{
		const std::vector<std::shared_ptr<IEntity>>* pDrawList;
		std::shared_ptr<IEntity> pCamera;
		BaseRenderer* pRenderer;
	};

	class RenderGraph;
	class RenderNode : std::enable_shared_from_this<RenderNode>
	{
	public:
		RenderNode(const std::shared_ptr<RenderGraph> pRenderGraph, void(*pRenderPassFunc)(const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext), const RenderGraphResource& input, const RenderGraphResource& output);
		void AddNextNode(std::shared_ptr<RenderNode> pNode);

	private:
		void Execute();
		// TODO: add function to query the completion state

	private:
		void(*m_pRenderPassFunc)(const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext);
		std::shared_ptr<RenderContext> m_pContext;
		RenderGraphResource m_input;
		RenderGraphResource m_output;
		std::vector<std::shared_ptr<RenderNode>> m_prevNodes;
		std::vector<std::shared_ptr<RenderNode>> m_nextNodes;

		friend class RenderGraph;
		std::shared_ptr<RenderGraph> m_pRenderGraph;
	};

	class RenderGraph : public NoCopy
	{
	public:
		void AddRenderNode(std::shared_ptr<RenderNode> pNode);
		void BeginRenderPasses(const std::shared_ptr<RenderContext> pContext);

	private:
		std::vector<std::shared_ptr<RenderNode>> m_nodes;
		std::queue<std::shared_ptr<RenderNode>>  m_startingNodes; // Nodes that has no previous dependencies
	};
}