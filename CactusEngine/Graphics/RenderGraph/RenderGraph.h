#pragma once
#include "Global.h"
#include "GraphicsDevice.h"
#include "BuiltInShaderType.h"
#include "NoCopy.h"
#include "SafeQueue.h"

#include <queue>
#include <mutex>

namespace Engine
{
	class BaseEntity;
	class BaseRenderer;
	class RenderGraph;

	class RenderGraphResource
	{
	public:
		void Add(const char* name, std::shared_ptr<RawResource> pResource);
		std::shared_ptr<RawResource> Get(const char* name) const;
		void Swap(const char* name, std::shared_ptr<RawResource> pResource);

	private:
		std::unordered_map<const char*, std::shared_ptr<RawResource>> m_renderResources;
	};

	struct RenderContext
	{
		const std::vector<std::shared_ptr<BaseEntity>>* pDrawList = nullptr;
		std::shared_ptr<BaseEntity> pCamera;
	};

	struct CommandContext
	{
		std::shared_ptr<GraphicsCommandPool> pCommandPool = nullptr;
		std::shared_ptr<GraphicsCommandPool> pTransferCommandPool = nullptr;
	};
	
	class RenderNode : std::enable_shared_from_this<RenderNode>
	{
	public:
		RenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void ConnectNext(std::shared_ptr<RenderNode> pNode);
		void SetInputResource(const char* slot, const char* pResourceName);

	protected:
		void Setup();
		void ExecuteSequential(); // For OpenGL
		void ExecuteParallel();	  // For Vulkan

		virtual void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) = 0;
		virtual void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) = 0;

	protected:
		const char*									m_pName;
		BaseRenderer*								m_pRenderer;
		std::shared_ptr<GraphicsDevice>				m_pDevice;
		EGraphicsAPIType							m_eGraphicsDeviceType;

		std::vector<RenderNode*>					m_prevNodes;
		std::vector<std::shared_ptr<RenderNode>>	m_nextNodes;
		bool										m_finishedExecution;

		std::shared_ptr<RenderGraphResource>		m_pGraphResources;
		std::shared_ptr<RenderContext>				m_pRenderContext;
		std::shared_ptr<CommandContext>				m_pCmdContext;
		std::unordered_map<const char*, const char*> m_inputResourceNames;
		std::unordered_map<EBuiltInShaderProgramType, std::shared_ptr<GraphicsPipelineObject>> m_graphicsPipelines;

		friend class RenderGraph;
	};

	class RenderGraph : public NoCopy
	{
	public:
		RenderGraph(const std::shared_ptr<GraphicsDevice> pDevice, uint32_t executionThreadCount);
		~RenderGraph();

		void AddRenderNode(const char* name, std::shared_ptr<RenderNode> pNode);
		void SetupRenderNodes();
		void BuildRenderNodePriorities();

		void BeginRenderPassesSequential(const std::shared_ptr<RenderContext> pContext);
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
		std::shared_ptr<GraphicsDevice> m_pDevice;
		std::unordered_map<const char*, std::shared_ptr<RenderNode>> m_nodes;
		std::queue<std::shared_ptr<RenderNode>> m_startingNodes; // Nodes that has no previous dependencies
		bool m_isRunning;

		// For parallel node execution	
		uint32_t m_executionThreadCount;
		std::vector<std::thread> m_executionThreads;
		std::mutex m_nodeExecutionMutex;
		std::condition_variable m_nodeExecutionCv;
		SafeQueue<std::shared_ptr<RenderNode>> m_executionNodeQueue;
	};
}