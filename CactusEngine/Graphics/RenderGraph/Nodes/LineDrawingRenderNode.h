#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class LineDrawingRenderNode : public RenderNode
	{
	public:
		LineDrawingRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_LINE_SPACE_TEXTURE;

	public:
		bool m_enableLineSmooth;

	private:
		FrameBuffer*		m_pFrameBuffer_Line;
		FrameBuffer*		m_pFrameBuffer_BlurHorizontal; // Optional
		FrameBuffer*		m_pFrameBuffer_BlurFinal;	   // Optional
		FrameBuffer*		m_pFrameBuffer_FinalBlend;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pControlVariables_UB;

		Texture2D*			m_pColorOutput;
		Texture2D*			m_pLineResult;
	};
}