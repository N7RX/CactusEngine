#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DeferredLightingRenderNode : public RenderNode
	{
	public:
		DeferredLightingRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

		void SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

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
				m_pTransformMatrices_UB(nullptr),
				m_pCameraMatrices_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pLightSourceProperties_UB(nullptr),
				m_pColorOutput(nullptr)
			{

			}

			FrameBuffer* m_pFrameBuffer;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pCameraMatrices_UB;
			UniformBuffer* m_pCameraProperties_UB;
			UniformBuffer* m_pLightSourceProperties_UB;

			Texture2D* m_pColorOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}