#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class OpaqueContentRenderNode : public RenderNode
	{
	public:
		OpaqueContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

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
		static const char* OUTPUT_COLOR_TEXTURE;
		static const char* OUTPUT_DEPTH_TEXTURE;

		static const char* INPUT_GBUFFER_NORMAL;
		static const char* INPUT_SHADOW_MAP;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pColorOutput(nullptr),
				m_pDepthOutput(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pCameraMatrices_UB(nullptr),
				m_pLightSpaceTransformMatrix_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pMaterialNumericalProperties_UB(nullptr)
			{

			}

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pColorOutput);
				CE_SAFE_DELETE(m_pDepthOutput);
				CE_SAFE_DELETE(m_pTransformMatrices_UB);
				CE_SAFE_DELETE(m_pCameraMatrices_UB);
				CE_SAFE_DELETE(m_pLightSpaceTransformMatrix_UB);
				CE_SAFE_DELETE(m_pCameraProperties_UB);
				CE_SAFE_DELETE(m_pMaterialNumericalProperties_UB);
			}

			FrameBuffer* m_pFrameBuffer;

			Texture2D* m_pColorOutput;
			Texture2D* m_pDepthOutput;

			UniformBuffer* m_pTransformMatrices_UB;
			UniformBuffer* m_pCameraMatrices_UB;
			UniformBuffer* m_pLightSpaceTransformMatrix_UB;
			UniformBuffer* m_pCameraProperties_UB;
			UniformBuffer* m_pMaterialNumericalProperties_UB;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}