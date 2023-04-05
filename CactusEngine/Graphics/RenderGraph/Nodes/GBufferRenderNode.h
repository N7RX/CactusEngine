#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class GBufferRenderNode : public RenderNode
	{
	public:
		GBufferRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

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
		void CreateMutableTextures(const RenderNodeConfiguration& initInfo);
		void CreateMutableBuffers(const RenderNodeConfiguration& initInfo);
		void DestroyMutableTextures();
		void DestroyMutableBuffers();

	public:
		static const char* OUTPUT_NORMAL_GBUFFER;
		static const char* OUTPUT_POSITION_GBUFFER;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pDepthBuffer(nullptr),
				m_pNormalOutput(nullptr),
				m_pPositionOutput(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pCameraMatrices_UB(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pDepthBuffer);
				CE_SAFE_DELETE(m_pNormalOutput);
				CE_SAFE_DELETE(m_pPositionOutput);
				CE_SAFE_DELETE(m_pTransformMatrices_UB);
				CE_SAFE_DELETE(m_pCameraMatrices_UB);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pDepthBuffer;
			Texture2D* m_pNormalOutput;
			Texture2D* m_pPositionOutput;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pCameraMatrices_UB;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}