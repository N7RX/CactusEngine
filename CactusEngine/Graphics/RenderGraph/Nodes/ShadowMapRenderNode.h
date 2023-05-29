#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class ShadowMapRenderNode : public RenderNode
	{
	public:
		ShadowMapRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

	protected:
		void CreateConstantResources(const RenderNodeConfiguration& initInfo) override;
		void CreateMutableResources(const RenderNodeConfiguration& initInfo) override;
		void DestroyMutableResources() override;

		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;

		void PrebuildGraphicsPipelines() override;

	private:
		void CreateMutableTextures(const RenderNodeConfiguration& initInfo);
		void DestroyMutableTextures();

	public:
		static const char* OUTPUT_DEPTH_TEXTURE;

	public:
		const uint32_t SHADOW_MAP_RESOLUTION = 2048; // TODO: move this into graphics config

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pDepthOutput(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pDepthOutput);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pDepthOutput;
		};
		std::vector<FrameResources> m_frameResources;
	};
}