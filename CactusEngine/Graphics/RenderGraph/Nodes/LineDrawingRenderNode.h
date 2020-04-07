#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class LineDrawingRenderNode : public RenderNode
	{
	public:
		LineDrawingRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_LINE_SPACE_TEXTURE;

	public:
		bool m_enableLineSmooth;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_Line;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_BlurHorizontal; // Optional
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_BlurFinal;	   // Optional
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_FinalBlend;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pControlVariables_UB;

		std::shared_ptr<Texture2D>			m_pColorOutput;
		std::shared_ptr<Texture2D>			m_pLineResult;
	};
}