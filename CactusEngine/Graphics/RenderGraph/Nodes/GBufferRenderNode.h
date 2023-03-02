#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class GBufferRenderNode : public RenderNode
	{
	public:
		GBufferRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_NORMAL_GBUFFER;
		static const char* OUTPUT_POSITION_GBUFFER;

	private:
		FrameBuffer*		m_pFrameBuffer;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pTransformMatrices_UB;

		Texture2D*			m_pDepthBuffer;
		Texture2D*			m_pNormalOutput;
		Texture2D*			m_pPositionOutput;
	};
}