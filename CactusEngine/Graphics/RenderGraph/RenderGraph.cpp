#include "RenderGraph.h"
#include "Global.h"
#include "BaseEntity.h"
#include "BaseRenderer.h"
#include "GraphicsDevice.h"
#include "LogUtility.h"
#include "RenderingSystem.h"

namespace Engine
{
	const uint32_t DEFAULT_MAXDRAWCALL = 256;

	void RenderGraphResource::Add(const char* name, RawResource* pResource)
	{
		m_renderResources[name] = pResource;
	}

	RawResource* RenderGraphResource::Get(const char* name) const
	{
		if (m_renderResources.find(name) != m_renderResources.end())
		{
			return m_renderResources.at(name);
		}
		LOG_ERROR((std::string)"Couldn't find resource: " + name);
		return nullptr;
	}

	RenderNode::RenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer)
		: m_pRenderer(pRenderer),
		m_pDevice(m_pRenderer->GetGraphicsDevice()),
		m_eGraphicsDeviceType(m_pDevice->GetGraphicsAPIType()),
		m_finishedExecution(false),
		m_pName(nullptr),
		m_graphResources(graphResources),
		m_frameIndex(0),
		m_pSwapchainImages(nullptr),
		m_outputToSwapchain(false),
		m_defaultPipelineStates{},
		m_renderContext{},
		m_cmdContext{},
		m_pRenderPassObject(nullptr),
		m_configuration()
	{
		
	}

	RenderNode::~RenderNode()
	{
		DestroyMutableResources();
		DestroyConstantResources();
	}

	void RenderNode::ConnectNext(RenderNode* pNode)
	{
		m_nextNodes.emplace_back(pNode);
		pNode->m_prevNodes.emplace_back(this);
	}

	void RenderNode::SetInputResource(const char* slot, const char* pResourceName)
	{
		DEBUG_ASSERT_CE(m_inputResourceNames.find(slot) != m_inputResourceNames.end());
		m_inputResourceNames.at(slot) = pResourceName;
	}

	void RenderNode::UseSwapchainImages(std::vector<Texture2D*>* pSwapchainImages)
	{
		DEBUG_ASSERT_CE(pSwapchainImages != nullptr);
		m_pSwapchainImages = pSwapchainImages;
		m_outputToSwapchain = true;
	}

	void RenderNode::Setup(const RenderNodeConfiguration& initInfo)
	{
		m_configuration = initInfo;

		CreateConstantResources(initInfo);
		CreateMutableResources(initInfo);
	}

	void RenderNode::ExecuteSequential()
	{
		for (auto& pNode : m_prevNodes)
		{
			if (!pNode->m_finishedExecution)
			{
				return;
			}
		}

		RenderPassFunction(m_graphResources[m_frameIndex], m_renderContext, m_cmdContext);
		m_finishedExecution = true;

		for (auto& pNode : m_nextNodes)
		{
			pNode->ExecuteSequential();
		}
	}

	void RenderNode::ExecuteParallel()
	{
		RenderPassFunction(m_graphResources[m_frameIndex], m_renderContext, m_cmdContext);
		m_finishedExecution = true;
	}

	PipelineVertexInputStateCreateInfo RenderNode::GetDefaultVertexInputStateCreateInfo() const
	{
		VertexInputBindingDescription vertexInputBindingDesc{};
		vertexInputBindingDesc.binding = 0;
		vertexInputBindingDesc.stride = VertexBufferCreateInfo::interleavedStride;
		vertexInputBindingDesc.inputRate = EVertexInputRate::PerVertex;

		VertexInputAttributeDescription positionAttributeDesc{};
		positionAttributeDesc.binding = vertexInputBindingDesc.binding;
		positionAttributeDesc.location = GraphicsDevice::ATTRIB_POSITION_LOCATION;
		positionAttributeDesc.offset = VertexBufferCreateInfo::positionOffset;
		positionAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription normalAttributeDesc{};
		normalAttributeDesc.binding = vertexInputBindingDesc.binding;
		normalAttributeDesc.location = GraphicsDevice::ATTRIB_NORMAL_LOCATION;
		normalAttributeDesc.offset = VertexBufferCreateInfo::normalOffset;
		normalAttributeDesc.format = ETextureFormat::RGB32F;

		VertexInputAttributeDescription texcoordAttributeDesc{};
		texcoordAttributeDesc.binding = vertexInputBindingDesc.binding;
		texcoordAttributeDesc.location = GraphicsDevice::ATTRIB_TEXCOORD_LOCATION;
		texcoordAttributeDesc.offset = VertexBufferCreateInfo::texcoordOffset;
		texcoordAttributeDesc.format = ETextureFormat::RG32F;

		VertexInputAttributeDescription tangentAttributeDesc{};
		tangentAttributeDesc.binding = vertexInputBindingDesc.binding;
		tangentAttributeDesc.location = GraphicsDevice::ATTRIB_TANGENT_LOCATION;
		tangentAttributeDesc.offset = VertexBufferCreateInfo::tangentOffset;
		tangentAttributeDesc.format = ETextureFormat::RGB32F;

		PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.bindingDescs = { vertexInputBindingDesc };
		vertexInputStateCreateInfo.attributeDescs = { positionAttributeDesc, normalAttributeDesc, texcoordAttributeDesc, tangentAttributeDesc };

		return vertexInputStateCreateInfo;
	}

	void RenderNode::DestroyConstantResources()
	{
		CE_SAFE_DELETE(m_pUniformBufferAllocator);
		CE_SAFE_DELETE(m_pRenderPassObject);
		DestroyGraphicsPipelines();
	}

	GraphicsPipelineObject* RenderNode::GetGraphicsPipeline(uint32_t key)
	{
		if (m_graphicsPipelines.find(key) != m_graphicsPipelines.end())
		{
			return m_graphicsPipelines.at(key);
		}
		else
		{
			GraphicsPipelineCreateInfo pipelineCreateInfo{};

			pipelineCreateInfo.pShaderProgram = m_pRenderer->GetRenderingSystem()->GetShaderProgramByType((EBuiltInShaderProgramType)key);
			pipelineCreateInfo.pVertexInputState = m_defaultPipelineStates.pVertexInputState;
			pipelineCreateInfo.pInputAssemblyState = m_defaultPipelineStates.pInputAssemblyState;
			pipelineCreateInfo.pColorBlendState = m_defaultPipelineStates.pColorBlendState;
			pipelineCreateInfo.pRasterizationState = m_defaultPipelineStates.pRasterizationState;
			pipelineCreateInfo.pDepthStencilState = m_defaultPipelineStates.pDepthStencilState;
			pipelineCreateInfo.pMultisampleState = m_defaultPipelineStates.pMultisampleState;
			pipelineCreateInfo.pViewportState = m_defaultPipelineStates.pViewportState;
			pipelineCreateInfo.pRenderPass = m_pRenderPassObject;

			GraphicsPipelineObject* pPipeline = nullptr;
			m_pDevice->CreateGraphicsPipelineObject(pipelineCreateInfo, pPipeline);
			m_graphicsPipelines.emplace(key, pPipeline);

			return pPipeline;
		}
	}

	void RenderNode::DestroyGraphicsPipelines()
	{
		for (auto pipeline : m_graphicsPipelines)
		{
			CE_DELETE(pipeline.second);
		}
	}

	RenderGraph::RenderGraph(GraphicsDevice* pDevice)
		: m_pDevice(pDevice),
		m_isRunning(true),
		m_executionThreadCount(0)
	{

	}

	RenderGraph::~RenderGraph()
	{
		m_isRunning = false;

		for (uint32_t i = 0; i < m_executionThreadCount; i++)
		{
			m_executionThreads[i].join();
		}
	}

	void RenderGraph::AddRenderNode(const char* name, RenderNode* pNode)
	{
		m_nodes.emplace(name, pNode);
		m_nodes[name]->m_pName = name;
	}

	void RenderGraph::SetupRenderNodes()
	{
		RenderNodeConfiguration initInfo{};

		initInfo.width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		initInfo.height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();
		initInfo.framesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		// TODO: read these from configuration
		initInfo.colorFormat = ETextureFormat::RGBA8_SRGB;
		initInfo.swapSurfaceFormat = ETextureFormat::BGRA8_SRGB;
		initInfo.depthFormat = ETextureFormat::D24;
		initInfo.renderScale = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetRenderScale();

		for (auto& node : m_nodes)
		{
			node.second->Setup(initInfo);
		}
	}

	void RenderGraph::BuildRenderNodePriorities()
	{
		m_renderNodePriorities.clear();
		m_nodePriorityDependencies.clear();

		// Find all starting nodes
		std::queue<RenderNode*> startingNodes;
		for (auto& pNode : m_nodes)
		{
			pNode.second->m_finishedExecution = false;

			if (pNode.second->m_prevNodes.empty())
			{
				startingNodes.push(pNode.second);
			}
		}

		// Traverse graph
		std::vector<RenderNode*> traverseResult;
		while (!startingNodes.empty())
		{
			TraverseRenderNode(startingNodes.front(), traverseResult);
			startingNodes.pop();
		}

		// Reset flag
		for (auto& pNode : m_nodes)
		{
			pNode.second->m_finishedExecution = false;
		}

		// Assign priority by traverse sequence
		uint32_t assignedPriority = 0;
		for (uint32_t i = 0; i < traverseResult.size(); i++)
		{
			m_renderNodePriorities[traverseResult[i]->m_pName] = assignedPriority;
			assignedPriority++;
		}

		// Record priority dependencies
		for (uint32_t i = 0; i < traverseResult.size(); i++)
		{
			m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]] = std::vector<uint32_t>();
			for (auto pNode : traverseResult[i]->m_prevNodes)
			{
				m_nodePriorityDependencies[m_renderNodePriorities[traverseResult[i]->m_pName]].emplace_back(m_renderNodePriorities[pNode->m_pName]);
			}
		}
	}

	void RenderGraph::PrebuildPipelines()
	{
		for (auto& node : m_nodes)
		{
			node.second->PrebuildGraphicsPipelines();
		}
	}

	void RenderGraph::InitExecutionThreads()
	{
		m_executionThreadCount = 4; // Default max 4 threads
		m_executionThreadCount = std::min<uint32_t>(std::thread::hardware_concurrency() - 2, m_executionThreadCount); // -2 for main thread and render thread
		m_executionThreadCount = std::max<uint32_t>(m_executionThreadCount, 1);
#if defined(DEBUG_MODE_CE)
		if (m_executionThreadCount == 1)
		{
			LOG_MESSAGE("Hardware concurrency is very low, render graph execution will be suboptimal.");
		}
#endif

		for (uint32_t i = 0; i < m_executionThreadCount; i++)
		{
			m_executionThreads.emplace_back(&RenderGraph::ExecuteRenderNodesParallel, this);
		}
	}

	void RenderGraph::BeginRenderPassesParallel(const RenderContext& context, uint32_t frameIndex)
	{
		DEBUG_ASSERT_MESSAGE_CE(m_executionThreadCount > 0, "Render graph threads are uninitialized.");

		for (auto& pNode : m_nodes)
		{
			pNode.second->m_renderContext = context;
			pNode.second->m_finishedExecution = false;
			pNode.second->m_frameIndex = frameIndex;

			if (pNode.second->m_prevNodes.empty())
			{
				m_startingNodes.push(pNode.second);
			}
		}

		{
			std::lock_guard<std::mutex> guard(m_nodeExecutionMutex);

			while (!m_startingNodes.empty())
			{
				EnqueueRenderNode(m_startingNodes.front());
				m_startingNodes.pop();
			}

			for (auto& pNode : m_nodes)
			{
				pNode.second->m_finishedExecution = false;
			}
		}

		m_nodeExecutionCv.notify_all();
	}

	RenderNode* RenderGraph::GetNodeByName(const char* name) const
	{
		if (m_nodes.find(name) != m_nodes.end())
		{
			return m_nodes.at(name);
		}
		LOG_ERROR((std::string)"Coundn't find node: " + name);
		return nullptr;
	}

	uint32_t RenderGraph::GetRenderNodeCount() const
	{
		return m_nodes.size();
	}

	void RenderGraph::UpdateResolution(uint32_t width, uint32_t height)
	{
		for (auto& pNode : m_nodes)
		{
			pNode.second->UpdateResolution(width, height);
		}
	}

	void RenderGraph::ExecuteRenderNodesParallel()
	{
		CommandContext cmdContext{};
		cmdContext.pCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Graphics);
		cmdContext.pTransferCommandPool = m_pDevice->RequestExternalCommandPool(EQueueType::Transfer);

		RenderNode* pNode = nullptr;
		while (m_isRunning)
		{
			{
				std::unique_lock<std::mutex> lock(m_nodeExecutionMutex);
				m_nodeExecutionCv.wait(lock, [this]() { return !m_executionNodeQueue.Empty(); });
			}

			while (m_executionNodeQueue.TryPop(pNode))
			{
				pNode->m_cmdContext = cmdContext;
				pNode->ExecuteParallel();

				std::this_thread::yield();
			}
		}
	}

	void RenderGraph::EnqueueRenderNode(RenderNode* pNode)
	{
		// Enqueue render nodes by dependency sequence
		for (auto& pPrevNode : pNode->m_prevNodes)
		{
			if (!pPrevNode->m_finishedExecution) // m_finishedExecution is being used as traversal flag here
			{
				return;
			}
		}

		m_executionNodeQueue.Push(pNode);
		pNode->m_finishedExecution = true;

		for (auto& pNextNode : pNode->m_nextNodes)
		{
			EnqueueRenderNode(pNextNode);
		}
	}

	void RenderGraph::TraverseRenderNode(RenderNode* pNode, std::vector<RenderNode*>& output)
	{
		// Record render nodes by dependency sequence
		for (auto& pPrevNode : pNode->m_prevNodes)
		{
			if (!pPrevNode->m_finishedExecution) // m_finishedExecution is being used as traversal flag here
			{
				return;
			}
		}

		output.emplace_back(pNode);
		pNode->m_finishedExecution = true;

		for (auto& pNextNode : pNode->m_nextNodes)
		{
			TraverseRenderNode(pNextNode, output);
		}
	}
}