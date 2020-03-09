#pragma once
#include "DrawingResources.h"
#include "IEntity.h"
#include "NoCopy.h"
#include "SafeQueue.h"
#include "CommandResources.h"
#include <queue>
#include <mutex>

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
		const std::vector<std::shared_ptr<IEntity>>* pDrawList = nullptr;
		std::shared_ptr<IEntity> pCamera;
		BaseRenderer* pRenderer = nullptr;
	};

	struct CommandContext
	{
		std::shared_ptr<DrawingCommandPool> pCommandPool = nullptr;
	};

	class RenderGraph;
	class RenderNode : std::enable_shared_from_this<RenderNode>
	{
	public:
		RenderNode(const std::shared_ptr<RenderGraph> pRenderGraph, 
			void(*pRenderPassFunc)(const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, std::shared_ptr<CommandContext> pCmdContext), 
			const RenderGraphResource& input, const RenderGraphResource& output);
		void AddNextNode(std::shared_ptr<RenderNode> pNode);

	private:
		void Execute();
		void ExecuteParallel();

	private:
		void(*m_pRenderPassFunc)(const RenderGraphResource& input, RenderGraphResource& output, const std::shared_ptr<RenderContext> pContext, const std::shared_ptr<CommandContext> pCmdContext);
		std::shared_ptr<RenderContext> m_pContext;
		std::shared_ptr<CommandContext> m_pCmdContext;
		RenderGraphResource m_input;
		RenderGraphResource m_output;
		std::vector<RenderNode*> m_prevNodes;
		std::vector<std::shared_ptr<RenderNode>> m_nextNodes;
		bool m_finishedExecution;

		const char* m_pName;

		friend class RenderGraph;
		std::shared_ptr<RenderGraph> m_pRenderGraph;
	};

	class RenderGraph : public NoCopy
	{
	public:
		RenderGraph(const std::shared_ptr<DrawingDevice> pDevice);
		~RenderGraph();

		void AddRenderNode(const char* name, std::shared_ptr<RenderNode> pNode);
		void BuildRenderNodePriorities();

		void BeginRenderPasses(const std::shared_ptr<RenderContext> pContext);
		void BeginRenderPassesParallel(const std::shared_ptr<RenderContext> pContext);

		std::shared_ptr<RenderNode> GetNodeByName(const char* name) const;
		uint32_t GetRenderNodeCount() const;

	private:
		void ExecuteRenderNodeParallel();
		void EnqueueRenderNode(const std::shared_ptr<RenderNode> pNode);
		void TraverseRenderNode(const std::shared_ptr<RenderNode> pNode, std::vector<std::shared_ptr<RenderNode>>& output);

	public:
		std::unordered_map<const char*, uint32_t> m_renderNodePriorities; // Render Node Name - Submit Priority
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_nodePriorityDependencies; // Submit Priority - Dependent Submit Priority

	private:
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::unordered_map<const char*, std::shared_ptr<RenderNode>> m_nodes;
		std::queue<std::shared_ptr<RenderNode>> m_startingNodes; // Nodes that has no previous dependencies

		// For parallel node execution
		bool m_isRunning;
		static const unsigned int EXECUTION_THREAD_COUNT = 4;
		std::thread m_executionThreads[EXECUTION_THREAD_COUNT];
		std::mutex m_nodeExecutionMutex;
		std::condition_variable m_nodeExecutionCv;
		SafeQueue<std::shared_ptr<RenderNode>> m_executionNodeQueue;
	};
}