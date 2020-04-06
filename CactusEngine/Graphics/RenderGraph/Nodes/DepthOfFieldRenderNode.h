#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DepthOfFieldRenderNode : public RenderNode
	{
	public:
		DepthOfFieldRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;
		static const char* INPUT_SHADOW_MARK_TEXTURE;

	private:
		std::shared_ptr<FrameBuffer>			m_pFrameBuffer_Horizontal;
		std::shared_ptr<SwapchainFrameBuffers>	m_pFrameBuffers_Present;
		std::shared_ptr<RenderPassObject>		m_pRenderPassObject_Horizontal;
		std::shared_ptr<RenderPassObject>		m_pRenderPassObject_Present;

		std::shared_ptr<UniformBuffer>		m_pTransformMatrices_UB;
		std::shared_ptr<UniformBuffer>		m_pSystemVariables_UB;
		std::shared_ptr<UniformBuffer>		m_pCameraProperties_UB;
		std::shared_ptr<UniformBuffer>		m_pControlVariables_UB;

		std::shared_ptr<Texture2D>			m_pHorizontalResult;
		std::shared_ptr<Texture2D>			m_pBrushMaskTexture_1;
		std::shared_ptr<Texture2D>			m_pBrushMaskTexture_2;
		std::shared_ptr<Texture2D>			m_pPencilMaskTexture_1;
		std::shared_ptr<Texture2D>			m_pPencilMaskTexture_2;
	};
}