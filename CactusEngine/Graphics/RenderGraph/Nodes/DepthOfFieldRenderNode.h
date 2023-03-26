#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DepthOfFieldRenderNode : public RenderNode
	{
	public:
		DepthOfFieldRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

	protected:
		void CreateConstantResources(const RenderNodeInitInfo& initInfo) override;
		void CreateMutableResources(const RenderNodeInitInfo& initInfo) override;

		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

		void DestroyMutableResources() override;
		void DestroyConstantResources() override;

	private:
		void HorizontalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);
		void VerticalPass(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);

		void CreateMutableTextures(const RenderNodeInitInfo& initInfo);
		void CreateMutableBuffers(const RenderNodeInitInfo& initInfo);
		void DestroyMutableTextures();
		void DestroyMutableBuffers();

	public:
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer_Horizontal(nullptr),
				m_pCameraMatrices_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pControlVariables_UB(nullptr),
				m_pHorizontalResult(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer_Horizontal);
				CE_SAFE_DELETE(m_pCameraMatrices_UB);
				CE_SAFE_DELETE(m_pCameraProperties_UB);
				CE_SAFE_DELETE(m_pControlVariables_UB);
				CE_SAFE_DELETE(m_pHorizontalResult);
			}

			FrameBuffer* m_pFrameBuffer_Horizontal;

			UniformBuffer* m_pCameraMatrices_UB;
			UniformBuffer* m_pCameraProperties_UB;
			UniformBuffer* m_pControlVariables_UB;

			Texture2D* m_pHorizontalResult;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject_Horizontal;
		RenderPassObject* m_pRenderPassObject_Present;

		SwapchainFrameBuffers m_frameBuffers_Present;
	};
}