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
		void Add(const char* name, RawResource* pResource);
		RawResource* Get(const char* name) const;
		void Swap(const char* name, RawResource* pResource);

	private:
		std::unordered_map<const char*, RawResource*> m_renderResources;
	};

	struct RenderContext
	{
		const std::vector<BaseEntity*>* pDrawList = nullptr;
		BaseEntity* pCamera;
	};

	struct CommandContext
	{
		GraphicsCommandPool* pCommandPool = nullptr;
		GraphicsCommandPool* pTransferCommandPool = nullptr;
	};
	
	class RenderNode
	{
	public:
		RenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void ConnectNext(RenderNode* pNode);
		void SetInputResource(const char* slot, const char* pResourceName);

	protected:
		void Setup();
		void ExecuteSequential(); // For OpenGL
		void ExecuteParallel();	  // For Vulkan

		virtual void SetupFunction(RenderGraphResource* pGraphResources) = 0;
		virtual void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) = 0;

	protected:
		const char*					m_pName;
		BaseRenderer*				m_pRenderer;
		GraphicsDevice*				m_pDevice;
		EGraphicsAPIType			m_eGraphicsDeviceType;

		std::vector<RenderNode*>	m_prevNodes;
		std::vector<RenderNode*>	m_nextNodes;
		bool						m_finishedExecution;

		RenderGraphResource*		m_pGraphResources;
		RenderContext*				m_pRenderContext;
		CommandContext*				m_pCmdContext;
		std::unordered_map<const char*, const char*> m_inputResourceNames;
		std::unordered_map<EBuiltInShaderProgramType, GraphicsPipelineObject*> m_graphicsPipelines;

		friend class RenderGraph;
	};

	class RenderGraph : public NoCopy
	{
	public:
		RenderGraph(GraphicsDevice* pDevice, uint32_t executionThreadCount);
		~RenderGraph();

		void AddRenderNode(const char* name, RenderNode* pNode);
		void SetupRenderNodes();
		void BuildRenderNodePriorities();

		void BeginRenderPassesSequential(RenderContext* pContext);
		void BeginRenderPassesParallel(RenderContext* pContext);

		RenderNode* GetNodeByName(const char* name) const;
		uint32_t GetRenderNodeCount() const;

	private:
		void ExecuteRenderNodeParallel();
		void EnqueueRenderNode(RenderNode* pNode);
		void TraverseRenderNode(RenderNode* pNode, std::vector<RenderNode*>& output);

	public:
		std::unordered_map<const char*, uint32_t> m_renderNodePriorities; // Render Node Name - Submit Priority
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_nodePriorityDependencies; // Submit Priority - Dependent Submit Priority

	private:
		GraphicsDevice* m_pDevice;
		std::unordered_map<const char*, RenderNode*> m_nodes;
		std::queue<RenderNode*> m_startingNodes; // Nodes that has no previous dependencies
		bool m_isRunning;

		// For parallel node execution	
		uint32_t m_executionThreadCount;
		std::vector<std::thread> m_executionThreads;
		std::mutex m_nodeExecutionMutex;
		std::condition_variable m_nodeExecutionCv;
		SafeQueue<RenderNode*> m_executionNodeQueue;
	};
}