#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DeferredLightingRenderNode : public RenderNode
	{
	public:
		DeferredLightingRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		
		static const char* INPUT_GBUFFER_COLOR;
		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_GBUFFER_POSITION;
		static const char* INPUT_DEPTH_TEXTURE;

	private:
		FrameBuffer*		m_pFrameBuffer;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pTransformMatrices_UB;
		UniformBuffer*		m_pCameraProperties_UB;
		UniformBuffer*		m_pLightSourceProperties_UB;

		Texture2D*			m_pColorOutput;
	};
}