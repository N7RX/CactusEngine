#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class GBufferRenderNode : public RenderNode
	{
	public:
		GBufferRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_NORMAL_GBUFFER;
		static const char* OUTPUT_POSITION_GBUFFER;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pTransformMatrices_UB;

		std::shared_ptr<Texture2D>			m_pDepthBuffer;
		std::shared_ptr<Texture2D>			m_pNormalOutput;
		std::shared_ptr<Texture2D>			m_pPositionOutput;
	};
}