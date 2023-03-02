#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class OpaqueContentRenderNode : public RenderNode
	{
	public:
		OpaqueContentRenderNode(RenderGraphResource* pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(RenderGraphResource* pGraphResources) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		static const char* OUTPUT_DEPTH_TEXTURE;
		static const char* OUTPUT_LINE_SPACE_TEXTURE;

		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_SHADOW_MAP;

	private:
		FrameBuffer*		m_pFrameBuffer;
		RenderPassObject*	m_pRenderPassObject;

		UniformBuffer*		m_pTransformMatrices_UB;
		UniformBuffer*		m_pLightSpaceTransformMatrix_UB;
		UniformBuffer*		m_pCameraProperties_UB;
		UniformBuffer*		m_pMaterialNumericalProperties_UB;

		Texture2D*			m_pColorOutput;
		Texture2D*			m_pDepthOutput;
		Texture2D*			m_pLineSpaceOutput;
	};
}