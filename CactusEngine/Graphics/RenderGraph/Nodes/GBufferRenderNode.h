#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class GBufferRenderNode : public RenderNode
	{
	public:
		GBufferRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

		void SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

	public:
		static const char* OUTPUT_NORMAL_GBUFFER;
		static const char* OUTPUT_POSITION_GBUFFER;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pCameraMatrices_UB(nullptr),
				m_pDepthBuffer(nullptr),
				m_pNormalOutput(nullptr),
				m_pPositionOutput(nullptr)
			{

			}

			FrameBuffer* m_pFrameBuffer;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pCameraMatrices_UB;

			Texture2D* m_pDepthBuffer;
			Texture2D* m_pNormalOutput;
			Texture2D* m_pPositionOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}