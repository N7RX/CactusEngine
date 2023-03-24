#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DepthOfFieldRenderNode : public RenderNode
	{
	public:
		DepthOfFieldRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

		void SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

	public:
		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_GBUFFER_POSITION;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer_Horizontal(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pControlVariables_UB(nullptr),
				m_pHorizontalResult(nullptr)
			{

			}

			FrameBuffer* m_pFrameBuffer_Horizontal;

			UniformBuffer* m_pTransformMatrices_UB;
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