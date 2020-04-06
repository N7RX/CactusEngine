#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class TransparencyBlendRenderNode : public RenderNode
	{
	public:
		TransparencyBlendRenderNode(std::shared_ptr<RenderGraphResource> pGraphResources, BaseRenderer* pRenderer);

		void SetupFunction(std::shared_ptr<RenderGraphResource> pGraphResources) override;
		void RenderPassFunction(std::shared_ptr<RenderGraphResource> pGraphResources, const std::shared_ptr<RenderContext> pRenderContext, const std::shared_ptr<CommandContext> pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;

		static const char* INPUT_OPQAUE_COLOR_TEXTURE;
		static const char* INPUT_OPQAUE_DEPTH_TEXTURE;
		static const char* INPUT_TRANSPARENCY_COLOR_TEXTURE;
		static const char* INPUT_TRANSPARENCY_DEPTH_TEXTURE;

	private:
		std::shared_ptr<FrameBuffer>		m_pFrameBuffer;
		std::shared_ptr<RenderPassObject>	m_pRenderPassObject;

		std::shared_ptr<Texture2D>			m_pColorOutput;
	};
}