#include "StandardRenderer.h"
#include "AllRenderNodes.h"
#include "RenderGraph.h"

namespace Engine
{
	StandardRenderer::StandardRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem)
		: BaseRenderer(ERendererType::Standard, pDevice, pSystem),
		m_renderThreadsCount(4)
	{

	}

	void StandardRenderer::BuildRenderGraph()
	{
#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Building standard render graph...");
#endif
		CE_NEW(m_pRenderGraph, RenderGraph, m_pDevice, m_renderThreadsCount);

		// Create required nodes

		ShadowMapRenderNode* pShadowMapNode;
		CE_NEW(pShadowMapNode, ShadowMapRenderNode, m_graphResources, this);
		GBufferRenderNode* pGBufferNode;
		CE_NEW(pGBufferNode, GBufferRenderNode, m_graphResources, this);
		OpaqueContentRenderNode* pOpaqueNode;
		CE_NEW(pOpaqueNode, OpaqueContentRenderNode, m_graphResources, this);
		DeferredLightingRenderNode* pDeferredLightingNode;
		CE_NEW(pDeferredLightingNode, DeferredLightingRenderNode, m_graphResources, this);
		TransparentContentRenderNode* pTransparencyNode;
		CE_NEW(pTransparencyNode, TransparentContentRenderNode, m_graphResources, this);
		TransparencyBlendRenderNode* pBlendNode;
		CE_NEW(pBlendNode, TransparencyBlendRenderNode, m_graphResources, this);

		m_pRenderGraph->AddRenderNode("ShadowMapNode", pShadowMapNode);
		m_pRenderGraph->AddRenderNode("GBufferNode", pGBufferNode);
		m_pRenderGraph->AddRenderNode("OpaqueNode", pOpaqueNode);
		m_pRenderGraph->AddRenderNode("DeferredLightingNode", pDeferredLightingNode);
		m_pRenderGraph->AddRenderNode("TransparencyNode", pTransparencyNode);
		m_pRenderGraph->AddRenderNode("BlendNode", pBlendNode);

		pShadowMapNode->ConnectNext(pOpaqueNode);
		pGBufferNode->ConnectNext(pOpaqueNode);
		pOpaqueNode->ConnectNext(pDeferredLightingNode);
		pDeferredLightingNode->ConnectNext(pTransparencyNode);
		pTransparencyNode->ConnectNext(pBlendNode);

		// Define resource dependencies

		ObtainSwapchainImages();

		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_SHADOW_MAP, ShadowMapRenderNode::OUTPUT_DEPTH_TEXTURE);
		pOpaqueNode->SetInputResource(OpaqueContentRenderNode::INPUT_GBUFFER_NORMAL, GBufferRenderNode::OUTPUT_NORMAL_GBUFFER);

		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_COLOR, OpaqueContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_NORMAL, GBufferRenderNode::OUTPUT_NORMAL_GBUFFER);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_GBUFFER_POSITION, GBufferRenderNode::OUTPUT_POSITION_GBUFFER);
		pDeferredLightingNode->SetInputResource(DeferredLightingRenderNode::INPUT_DEPTH_TEXTURE, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_COLOR_TEXTURE, DeferredLightingRenderNode::OUTPUT_COLOR_TEXTURE);
		pTransparencyNode->SetInputResource(TransparentContentRenderNode::INPUT_BACKGROUND_DEPTH, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_COLOR_TEXTURE, DeferredLightingRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_OPQAUE_DEPTH_TEXTURE, OpaqueContentRenderNode::OUTPUT_DEPTH_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_COLOR_TEXTURE, TransparentContentRenderNode::OUTPUT_COLOR_TEXTURE);
		pBlendNode->SetInputResource(TransparencyBlendRenderNode::INPUT_TRANSPARENCY_DEPTH_TEXTURE, TransparentContentRenderNode::OUTPUT_DEPTH_TEXTURE);

		pBlendNode->UseSwapchainImages(&m_swapchainImages);

		// Initialize render graph

		m_pRenderGraph->SetupRenderNodes();
		m_pRenderGraph->BuildRenderNodePriorities();

		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetPrebuildShadersAndPipelines())
		{
			m_pRenderGraph->PrebuildPipelines();
		}

		for (uint32_t i = 0; i < m_pRenderGraph->GetRenderNodeCount(); i++)
		{
			m_commandRecordReadyList.emplace(i, nullptr);
			m_commandRecordReadyListFlag.emplace(i, false);
		}

#if defined(DEVELOPMENT_MODE_CE)
		LOG_MESSAGE("Build standard render graph completed.");
#endif
	}
}