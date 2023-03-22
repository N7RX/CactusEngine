#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class TransparencyBlendRenderNode : public RenderNode
	{
	public:
		TransparencyBlendRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

		void SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

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

			FrameBuffer*		m_pFrameBuffer;

			Texture2D*			m_pColorOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}