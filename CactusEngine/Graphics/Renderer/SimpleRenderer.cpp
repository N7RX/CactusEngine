#include "SimpleRenderer.h"
#include "AllRenderNodes.h"
#include "RenderGraph.h"
#include "ImageTexture.h"

namespace Engine
{
	SimpleRenderer::SimpleRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem)
		: BaseRenderer(ERendererType::Simple, pDevice, pSystem),
		m_pDummyInputTexture(nullptr)
	{

	}

	SimpleRenderer::~SimpleRenderer()
	{
		CE_DELETE(m_pDummyInputTexture);
	}

	void SimpleRenderer::BuildRenderGraph()
	{
#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Building simple render graph...");
#endif
		CE_NEW(m_pRenderGraph, RenderGraph, m_pDevice);

		// Create required nodes

		GBufferRenderNode* pGBufferNode;
		CE_NEW(pGBufferNode, GBufferRenderNode, m_graphResources, this);
		OpaqueContentRenderNode* pOpaqueNode;
		CE_NEW(pOpaqueNode, OpaqueContentRenderNode, m_graphResources, this);
		TransparentContentRenderNode* pTransparencyNode;
		CE_NEW(pTransparencyNode, TransparentContentRenderNode, m_graphResources, this);
		TransparencyBlendRenderNode* pBlendNode;
		CE_NEW(pBlendNode, TransparencyBlendRenderNode, m_graphResources, this);

		m_pRenderGraph->AddRenderNode("GBufferNode", pGBufferNode);
		m_pRenderGraph->AddRenderNode("OpaqueNode", pOpaqueNode);
		m_pRenderGraph->AddRenderNode("TransparencyNode", pTransparencyNode);
		m_pRenderGraph->AddRenderNode("BlendNode", pBlendNode);

		pGBufferNode->ConnectNext(pOpaqueNode);
		pOpaqueNode->ConnectNext(pTransparencyNode);
		pTransparencyNode->ConnectNext(pBlendNode);

		// Define resource dependencies

		BuildDummyResources();
		ObtainSwapchainImages();

		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_SHADOW_MAP, ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE);
		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_GBUFFER_NORMAL, GBufferRenderNode::OUTPUT_NORMAL_GBUFFER);

		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_COLOR_TEXTURE, OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_BACKGROUND_DEPTH, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_COLOR_TEXTURE, OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_DEPTH_TEXTURE, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_COLOR_TEXTURE, TransparentContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_DEPTH_TEXTURE, TransparentContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlendNode->UseSwapchainImages(&m_swapchainImages);

		// Initialize render graph

		m_pRenderGraph->InitExecutionThreads();

		m_pRenderGraph->SetupRenderNodes();
		m_pRenderGraph->BuildRenderNodePriorities();

		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetPrebuildShadersAndPipelines())
		{
			m_pRenderGraph->PrebuildPipelines();
		}

		m_commandRecordReadyList.resize(m_pRenderGraph->GetRenderNodeCount());

#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Build simple render graph completed.");
#endif
	}

	void SimpleRenderer::BuildDummyResources()
	{
		CE_NEW(m_pDummyInputTexture, ImageTexture, "Assets/Textures/Default.png");
		m_pDummyInputTexture->SetSampler(m_pDevice->GetTextureSampler(ESamplerAnisotropyLevel::None));

		for (uint32_t i = 0; i < m_graphResources.size(); ++i)
		{
			m_graphResources[i]->Add(ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE, m_pDummyInputTexture);
		}
	}
}