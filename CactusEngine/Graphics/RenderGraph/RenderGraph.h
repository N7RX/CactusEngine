#pragma once
#include "DrawingResources.h"
#include "IEntity.h"
#include "NoCopy.h"
#include <queue>

namespace Engine
{
	class RenderGraphResource
	{
	public:
		void Add(const char* name, std::shared_ptr<RawResource> pResource);
		std::shared_ptr<RawResource> Get(const char* name) const;

	private:
		std::unordered_map<const char*, std::shared_ptr<RawResource>> m_renderResources;
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
		std::vector<RenderNode*> m_prevNodes;
		std::vector<std::shared_ptr<RenderNode>> m_nextNodes;

		friend class RenderGraph;
		std::shared_ptr<RenderGraph> m_pRenderGraph;
	};

	enum ERenderNodeType
	{
		eRenderNode_Opaque = 0,
		eRenderNode_Transparent,
		eRenderNode_ColorBlend_DepthBased_2,
		ERENDERNODETYPE_COUNT
	};

	class RenderGraph : public NoCopy
	{
	public:
		void AddRenderNode(ERenderNodeType type, std::shared_ptr<RenderNode> pNode);
		void BeginRenderPasses(const std::shared_ptr<RenderContext> pContext);

		std::shared_ptr<RenderNode> GetNodeByType(ERenderNodeType type) const;

	private:
		std::unordered_map<ERenderNodeType, std::shared_ptr<RenderNode>> m_nodes;
		std::queue<std::shared_ptr<RenderNode>>  m_startingNodes; // Nodes that has no previous dependencies
	};

	namespace RGraphResName
	{
		static const char* FWD_OPAQUE_FB = "OpaqueFrameBuffer";
		static const char* FWD_OPAQUE_COLOR = "OpaqueColor";
		static const char* FWD_OPAQUE_DEPTH = "OpaqueDepth";

		static const char* FWD_TRANSP_FB = "TranspFrameBuffer";
		static const char* FWD_TRANSP_COLOR = "TranspColor";
		static const char* FWD_TRANSP_DEPTH = "TranspDepth";
	}
}