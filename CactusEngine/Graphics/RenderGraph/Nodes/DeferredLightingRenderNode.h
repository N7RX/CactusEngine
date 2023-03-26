#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class DeferredLightingRenderNode : public RenderNode
	{
	public:
		DeferredLightingRenderNode(std::vector<RenderGraphResource*>& graphResources, BaseRenderer* pRenderer);

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
		void DirectionalLighting(RenderGraphResource* pGraphResources, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);
		void RegularLighting(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, GraphicsCommandBuffer* pCommandBuffer, ShaderParameterTable& shaderParamTable);

		void CreateMutableTextures(const RenderNodeInitInfo& initInfo);
		void CreateMutableBuffers(const RenderNodeInitInfo& initInfo);
		void DestroyMutableTextures();
		void DestroyMutableBuffers();

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

			~FrameResources()
			{
				CE_SAFE_DELETE(m_pFrameBuffer);
				CE_SAFE_DELETE(m_pTransformMatrices_UB);
				CE_SAFE_DELETE(m_pCameraMatrices_UB);
				CE_SAFE_DELETE(m_pCameraProperties_UB);
				CE_SAFE_DELETE(m_pLightSourceProperties_UB);
				CE_SAFE_DELETE(m_pColorOutput);
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