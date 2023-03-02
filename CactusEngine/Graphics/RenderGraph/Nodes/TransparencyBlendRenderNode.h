#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class TransparencyBlendRenderNode : public RenderNode
	{
	public:
		TransparencyBlendRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_OPQAUE_COLOR_TEXTURE;
		static const char* INPUT_OPQAUE_DEPTH_TEXTURE;
		static const char* INPUT_TRANSPARENCY_COLOR_TEXTURE;
		static const char* INPUT_TRANSPARENCY_DEPTH_TEXTURE;

	private:
		FrameBuffer*		m_pFrameBuffer;
		RenderPassObject*	m_pRenderPassObject;

		Texture2D*			m_pColorOutput;
	};
}