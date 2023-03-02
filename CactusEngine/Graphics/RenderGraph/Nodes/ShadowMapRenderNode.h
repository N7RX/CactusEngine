#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class ShadowMapRenderNode : public RenderNode
	{
	public:
		ShadowMapRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_DEPTH_TEXTURE;

	public:
		const uint32_t SHADOW_MAP_RESOLUTION = 4096;

	private:
		FrameBuffer*		m_pFrameBuffer;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pTransformMatrices_UB;
		UniformBuffer*		m_pLightSpaceTransformMatrix_UB;

		Texture2D*			m_pDepthOutput;
	};
}