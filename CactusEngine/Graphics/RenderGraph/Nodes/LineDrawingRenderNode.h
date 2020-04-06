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

	private:
		void CreateMatrixTexture();

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_BLURRED_COLOR_TEXTURE;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_Curvature;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_RidgeSearch;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_BlurHorizontal;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_BlurFinal;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_FinalBlend;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pControlVariables_UB;

		std::shared_ptr<Texture2D>			m_pColorOutput;
		std::shared_ptr<Texture2D>			m_pMatrixTexture;
		std::shared_ptr<Texture2D>			m_pCurvatureTexture;
		std::shared_ptr<Texture2D>			m_pBlurLineResult;
	};
}