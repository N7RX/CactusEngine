#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class BlurRenderNode : public RenderNode
	{
	public:
		BlurRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;

	private:
		FrameBuffer*		m_pFrameBuffer_Horizontal;
		FrameBuffer*		m_pFrameBuffer_Final;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pControlVariables_UB;

		Texture2D*			m_pHorizontalResult;
		Texture2D*			m_pColorOutput;
	};
}