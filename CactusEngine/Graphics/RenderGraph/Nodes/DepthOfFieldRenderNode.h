#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DepthOfFieldRenderNode : public RenderNode
	{
	public:
		DepthOfFieldRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;

	private:
		FrameBuffer*			m_pFrameBuffer_Horizontal;
		SwapchainFrameBuffers*	m_pFrameBuffers_Present;
		RenderPassObject*		m_pRenderPassObject_Horizontal;
		RenderPassObject*		m_pRenderPassObject_Present;

		UniformBuffer*		m_pTransformMatrices_UB;
		UniformBuffer*		m_pCameraProperties_UB;
		UniformBuffer*		m_pControlVariables_UB;

		Texture2D*			m_pHorizontalResult;
	};
}