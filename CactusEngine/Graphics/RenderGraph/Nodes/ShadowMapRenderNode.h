#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class ShadowMapRenderNode : public RenderNode
	{
	public:
		ShadowMapRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

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
		void CreateMutableTextures(const RenderNodeInitInfo& initInfo);
		void CreateMutableBuffers(const RenderNodeInitInfo& initInfo);
		void DestroyMutableTextures();
		void DestroyMutableBuffers();

	public:
		static const char* OUTPUT_DEPTH_TEXTURE;

	public:
		const uint32_t SHADOW_MAP_RESOLUTION = 2048; // TODO: move this into graphics config

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pLightSpaceTransformMatrix_UB(nullptr),
				m_pDepthOutput(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pTransformMatrices_UB);
				CE_SAFE_DELETE(m_pLightSpaceTransformMatrix_UB);
				CE_SAFE_DELETE(m_pDepthOutput);
			}

			FrameBuffer* m_pFrameBuffer;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pLightSpaceTransformMatrix_UB;

			Texture2D* m_pDepthOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}