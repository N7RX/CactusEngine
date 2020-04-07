#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class OpaqueContentRenderNode : public RenderNode
	{
	public:
		OpaqueContentRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		static const char* OUTPUT_DEPTH_TEXTURE;
		static const char* OUTPUT_SHADOW_MARK_TEXTURE;

		static const char* INPUT_GBUFFER_POSITION;
		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_SHADOW_MAP;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<UniformBuffer>		m_pTransformMatrices_UB;
		std::shared_ptr<UniformBuffer>		m_pLightSpaceTransformMatrix_UB;
		std::shared_ptr<UniformBuffer>		m_pCameraProperties_UB;
		std::shared_ptr<UniformBuffer>		m_pMaterialNumericalProperties_UB;	

		std::shared_ptr<Texture2D>			m_pColorOutput;
		std::shared_ptr<Texture2D>			m_pDepthOutput;
		std::shared_ptr<Texture2D>			m_pShadowMarkOutput;
	};
}