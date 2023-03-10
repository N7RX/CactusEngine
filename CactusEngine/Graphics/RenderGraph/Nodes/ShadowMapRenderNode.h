#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class ShadowMapRenderNode : public RenderNode
	{
	public:
		ShadowMapRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

		void SetupFunction() override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_DEPTH_TEXTURE;

	public:
		const uint32_t SHADOW_MAP_RESOLUTION = 4096;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pLightSpaceTransformMatrix_UB(nullptr),
				m_pDepthOutput(nullptr)
			{

			}

			FrameBuffer* m_pFrameBuffer;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pLightSpaceTransformMatrix_UB;

			Texture2D* m_pDepthOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}