#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DeferredLightingRenderNode : public RenderNode
	{
	public:
		DeferredLightingRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

	protected:
		void CreateConstantResources(const RenderNodeConfiguration& initInfo) override;
		void CreateMutableResources(const RenderNodeConfiguration& initInfo) override;
		void DestroyMutableResources() override;

		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;

		void PrebuildGraphicsPipelines() override;
		GraphicsPipelineObject* GetGraphicsPipeline(uint32_t key) override;

	private:
		void DirectionalLighting(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);
		void RegularLighting(RenderGraphResource* pGraphResources, const RenderContext& renderContext, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);

		void CreateMutableTextures(const RenderNodeConfiguration& initInfo);
		void DestroyMutableTextures();

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		
		static const char* INPUT_GBUFFER_COLOR;
		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_GBUFFER_POSITION;
		static const char* INPUT_DEPTH_TEXTURE;

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

		PipelineVertexInputState*   m_pVertexInputState_Empty;
		PipelineInputAssemblyState* m_pInputAssemblyState_Strip;
		PipelineRasterizationState* m_pRasterizationState_CullFront;
		PipelineColorBlendState*    m_pColorBlendState_NoBlend;
	};
}