#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class ShadowMapRenderNode : public RenderNode
	{
	public:
		ShadowMapRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_DEPTH_TEXTURE;

	public:
		const uint32_t SHADOW_MAP_RESOLUTION = 4096;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pTransformMatrices_UB;
		std::shared_ptr<UniformBuffer>		m_pLightSpaceTransformMatrix_UB;

		std::shared_ptr<Texture2D>			m_pDepthOutput;
	};
}