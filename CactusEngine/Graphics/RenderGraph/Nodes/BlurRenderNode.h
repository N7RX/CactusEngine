#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class BlurRenderNode : public RenderNode
	{
	public:
		BlurRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_Horizontal;
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer_Final;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pControlVariables_UB;

		std::shared_ptr<Texture2D>			m_pHorizontalResult;
		std::shared_ptr<Texture2D>			m_pColorOutput;
	};
}