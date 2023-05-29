#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DepthOfFieldRenderNode : public RenderNode
	{
	public:
		DepthOfFieldRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

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
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pColorInputMipmap(nullptr),
				m_pColorOutput(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pColorInputMipmap);
				CE_SAFE_DELETE(m_pColorOutput);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pColorInputMipmap; // Copy of the color input texture with mipmap
			Texture2D* m_pColorOutput;
		};
		std::vector<FrameResources> m_frameResources;
	};
}