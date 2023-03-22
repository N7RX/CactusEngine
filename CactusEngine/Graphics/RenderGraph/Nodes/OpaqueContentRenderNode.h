#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class OpaqueContentRenderNode : public RenderNode
	{
	public:
		OpaqueContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

		void SetupFunction(uint32_t width, uint32_t height, uint32_t maxDrawCall, uint32_t framesInFlight) override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

		void UpdateResolution(uint32_t width, uint32_t height) override;
		void UpdateMaxDrawCallCount(uint32_t count) override;
		void UpdateFramesInFlight(uint32_t framesInFlight) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		static const char* OUTPUT_DEPTH_TEXTURE;
		static const char* OUTPUT_LINE_SPACE_TEXTURE;

		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_SHADOW_MAP;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pLightSpaceTransformMatrix_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pMaterialNumericalProperties_UB(nullptr),
				m_pColorOutput(nullptr),
				m_pDepthOutput(nullptr),
				m_pLineSpaceOutput(nullptr)
			{

			}

			FrameBuffer* m_pFrameBuffer;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pLightSpaceTransformMatrix_UB;
			UniformBuffer* m_pCameraProperties_UB;
			UniformBuffer* m_pMaterialNumericalProperties_UB;

			Texture2D* m_pColorOutput;
			Texture2D* m_pDepthOutput;
			Texture2D* m_pLineSpaceOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}