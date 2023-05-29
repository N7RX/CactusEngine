#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class GBufferRenderNode : public RenderNode
	{
	public:
		GBufferRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

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
		static const char* OUTPUT_NORMAL_GBUFFER;
		static const char* OUTPUT_POSITION_GBUFFER;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pDepthBuffer(nullptr),
				m_pNormalOutput(nullptr),
				m_pPositionOutput(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pDepthBuffer);
				CE_SAFE_DELETE(m_pNormalOutput);
				CE_SAFE_DELETE(m_pPositionOutput);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pDepthBuffer;
			Texture2D* m_pNormalOutput;
			Texture2D* m_pPositionOutput;
		};
		std::vector<FrameResources> m_frameResources;
	};
}