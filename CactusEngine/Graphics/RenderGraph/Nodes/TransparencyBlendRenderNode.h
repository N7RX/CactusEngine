#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class TransparencyBlendRenderNode : public RenderNode
	{
	public:
		TransparencyBlendRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

	protected:
		void CreateConstantResources(const RenderNodeConfiguration& initInfo) override;
		void CreateMutableResources(const RenderNodeConfiguration& initInfo) override;

		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

		void DestroyMutableResources() override;

		void PrebuildGraphicsPipelines() override;

	private:
		void CreateMutableTextures(const RenderNodeConfiguration& initInfo);
		void DestroyMutableTextures();

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_OPQAUE_COLOR_TEXTURE;
		static const char* INPUT_OPQAUE_DEPTH_TEXTURE;
		static const char* INPUT_TRANSPARENCY_COLOR_TEXTURE;
		static const char* INPUT_TRANSPARENCY_DEPTH_TEXTURE;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pColorOutput(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pColorOutput);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pColorOutput;
		};
		std::vector<FrameResources> m_frameResources;
	};
}