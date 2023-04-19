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

	private:
		std::unordered_map<const char*, RawResource*> m_renderResources;
	};

	struct RenderContext
	{
		const std::vector<BaseEntity*>* pOpaqueDrawList;
		const std::vector<BaseEntity*>* pTransparentDrawList;
		const std::vector<BaseEntity*>* pLightDrawList;
		const BaseEntity* pCamera;
	};

	struct CommandContext
	{
		GraphicsCommandPool* pCommandPool;
		GraphicsCommandPool* pTransferCommandPool;
	};
	
	struct RenderNodeConfiguration
	{
		uint32_t width;
		uint32_t height;
		uint32_t maxDrawCall;
		uint32_t framesInFlight;
		ETextureFormat colorFormat;
		ETextureFormat swapSurfaceFormat;
		ETextureFormat depthFormat;
		float renderScale;
	};

	class RenderNode
	{
	public:
		RenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);
		virtual ~RenderNode();

		void ConnectNext(RenderNode* pNode);
		void SetInputResource(const char* slot, const char* pResourceName);

		void UseSwapchainImages(std::vector<Texture2D*>* pSwapchainImages);

	protected:
		void Setup(const RenderNodeConfiguration& initInfo);

		void ExecuteSequential(); // For OpenGL
		void ExecuteParallel();	  // For Vulkan

		virtual void CreateConstantResources(const RenderNodeConfiguration& initInfo) = 0; // Pipeline objects that are constant
		virtual void CreateMutableResources(const RenderNodeConfiguration& initInfo) = 0;  // Buffers, render textures, etc. that can be changed

		virtual void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext) = 0;

		virtual void UpdateResolution(uint32_t width, uint32_t height) = 0;
		virtual void UpdateMaxDrawCallCount(uint32_t count) = 0; // May affect the size of uniform buffers
		virtual void UpdateFramesInFlight(uint32_t framesInFlight) = 0;

		virtual void DestroyMutableResources() {}
		virtual void DestroyConstantResources();

		virtual void PrebuildGraphicsPipelines() = 0;
		virtual GraphicsPipelineObject* GetGraphicsPipeline(uint32_t key);
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

		std::vector<Texture2D*>* m_pSwapchainImages;
		bool m_outputToSwapchain;

		struct DefaultGraphicsPipelineStates
		{
			DefaultGraphicsPipelineStates()
				: pVertexInputState(nullptr)
				, pInputAssemblyState(nullptr)
				, pRasterizationState(nullptr)
				, pMultisampleState(nullptr)
				, pDepthStencilState(nullptr)
				, pColorBlendState(nullptr)
				, pViewportState(nullptr)
			{}

			~DefaultGraphicsPipelineStates()
			{
				CE_SAFE_DELETE(pVertexInputState);
				CE_SAFE_DELETE(pInputAssemblyState);
				CE_SAFE_DELETE(pRasterizationState);
				CE_SAFE_DELETE(pMultisampleState);
				CE_SAFE_DELETE(pDepthStencilState);
				CE_SAFE_DELETE(pColorBlendState);
				CE_SAFE_DELETE(pViewportState);
			}

			PipelineVertexInputState*	pVertexInputState;
			PipelineInputAssemblyState*	pInputAssemblyState;
			PipelineRasterizationState* pRasterizationState;
			PipelineMultisampleState*	pMultisampleState;
			PipelineDepthStencilState*	pDepthStencilState;
			PipelineColorBlendState*	pColorBlendState;
			PipelineViewportState*		pViewportState;
		};
		DefaultGraphicsPipelineStates m_defaultPipelineStates;

		RenderContext  m_renderContext;
		CommandContext m_cmdContext;

		RenderPassObject* m_pRenderPassObject; // This can be null if a node is compute only

		std::unordered_map<const char*, const char*> m_inputResourceNames;
		std::unordered_map<uint32_t, GraphicsPipelineObject*> m_graphicsPipelines; // Key usually is the shader type, but ultimately it's determined by each render node
																				   // (e.g. might reuse same shader with a different render pass)

		RenderNodeConfiguration m_configuration;

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
		void PrebuildPipelines();

		void BeginRenderPassesSequential(const RenderContext& context, uint32_t frameIndex);
		void BeginRenderPassesParallel(const RenderContext& context, uint32_t frameIndex);

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