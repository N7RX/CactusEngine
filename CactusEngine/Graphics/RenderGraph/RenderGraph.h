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
	
	struct RenderNodeInitInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t maxDrawCall;
		uint32_t framesInFlight;
		// TODO: configure color and depth format etc.
	};

	class RenderNode
	{
	public:
		RenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);
		virtual ~RenderNode();

		void ConnectNext(RenderNode* pNode);
		void SetInputResource(const char* slot, const char* pResourceName);

	protected:
		void Setup(const RenderNodeInitInfo& initInfo);
		void ExecuteSequential(); // For OpenGL
		void ExecuteParallel();	  // For Vulkan

		virtual void CreateConstantResources(const RenderNodeInitInfo& initInfo) = 0; // Pipeline objects that are constant
		virtual void CreateMutableResources(const RenderNodeInitInfo& initInfo) = 0;  // Buffers, render textures, etc. that can be changed

		virtual void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) = 0;

		virtual void UpdateResolution(uint32_t width, uint32_t height) = 0;
		virtual void UpdateMaxDrawCallCount(uint32_t count) = 0; // May affect the size of uniform buffers
		virtual void UpdateFramesInFlight(uint32_t framesInFlight) = 0;

		virtual void DestroyMutableResources() {}
		virtual void DestroyConstantResources() {}

		void DestroyGraphicsPipelines();

	protected:
		const char*					m_pName;
		BaseRenderer*				m_pRenderer;
		GraphicsDevice*				m_pDevice;
		EGraphicsAPIType			m_eGraphicsDeviceType;

		std::vector<RenderNode*>	m_prevNodes;
		std::vector<RenderNode*>	m_nextNodes;
		bool						m_finishedExecution;

		std::vector<RenderGraphResource*> m_graphResources;
		uint32_t m_frameIndex;

		RenderContext* m_pRenderContext;
		CommandContext*	m_pCmdContext;
		std::unordered_map<const char*, const char*> m_inputResourceNames;
		std::unordered_map<EBuiltInShaderProgramType, GraphicsPipelineObject*> m_graphicsPipelines;

		RenderNodeInitInfo m_configuration;

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

		void BeginRenderPassesSequential(RenderContext* pContext, uint32_t frameIndex);
		void BeginRenderPassesParallel(RenderContext* pContext, uint32_t frameIndex);

		RenderNode* GetNodeByName(const char* name) const;
		uint32_t GetRenderNodeCount() const;

		void UpdateResolution(uint32_t width, uint32_t height);
		void UpdateFramesInFlight(uint32_t framesInFlight);

	private:
		void ExecuteRenderNodesParallel();
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