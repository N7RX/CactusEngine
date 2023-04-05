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

		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext& renderContext, const CommandContext& cmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

		void DestroyMutableResources() override;
		void DestroyConstantResources() override;

	private:
		void HorizontalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);
		void VerticalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);

		void CreateMutableTextures(const RenderNodeConfiguration& initInfo);
		void CreateMutableBuffers(const RenderNodeConfiguration& initInfo);
		void DestroyMutableTextures();
		void DestroyMutableBuffers();

	public:
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;

	private:
		const uint32_t VERTICAL_PASS_PIPELINE_KEY = 165536; // Just a value that ensures we don't have a collision with the horizontal pass pipeline key

		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer_Horizontal(nullptr),
				m_pFrameBuffer_Vertical(nullptr),
				m_pHorizontalResult(nullptr),
				m_pVerticalResult(nullptr),
				m_pCameraMatrices_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pControlVariables_UB(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer_Horizontal);
				CE_SAFE_DELETE(m_pFrameBuffer_Vertical);
				CE_SAFE_DELETE(m_pHorizontalResult);
				CE_SAFE_DELETE(m_pVerticalResult);
				CE_SAFE_DELETE(m_pCameraMatrices_UB);
				CE_SAFE_DELETE(m_pCameraProperties_UB);
				CE_SAFE_DELETE(m_pControlVariables_UB);
			}

			FrameBuffer* m_pFrameBuffer_Horizontal;
			FrameBuffer* m_pFrameBuffer_Vertical;

			Texture2D* m_pHorizontalResult;
			Texture2D* m_pVerticalResult;

			UniformBuffer* m_pCameraMatrices_UB;
			UniformBuffer* m_pCameraProperties_UB;
			UniformBuffer* m_pControlVariables_UB;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject_Horizontal;
		RenderPassObject* m_pRenderPassObject_Vertical;
	};
}