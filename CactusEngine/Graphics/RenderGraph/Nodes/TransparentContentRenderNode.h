#pragma once
#include "RenderGraph.h"

namespace Engine
{
	class TransparentContentRenderNode : public RenderNode
	{
	public:
		TransparentContentRenderNode(std::vector<RenderGraphResource*> graphResources, BaseRenderer* pRenderer);

		void SetupFunction() override;
		void RenderPassFunction(RenderGraphResource* pGraphResources, const RenderContext* pRenderContext, const CommandContext* pCmdContext) override;

	public:
		static const char* OUTPUT_COLOR_TEXTURE;
		static const char* OUTPUT_DEPTH_TEXTURE;

		static const char* INPUT_COLOR_TEXTURE;
		static const char* INPUT_BACKGROUND_DEPTH;

	private:
		struct FrameResources
		{
			FrameResources()
				: m_pFrameBuffer(nullptr),
				m_pTransformMatrices_UB(nullptr),
				m_pSystemVariables_UB(nullptr),
				m_pCameraProperties_UB(nullptr),
				m_pMaterialNumericalProperties_UB(nullptr),
				m_pColorOutput(nullptr),
				m_pDepthOutput(nullptr)
			{

			}

			FrameBuffer*		m_pFrameBuffer;

			UniformBuffer*		m_pTransformMatrices_UB;
			UniformBuffer*		m_pSystemVariables_UB;
			UniformBuffer*		m_pCameraProperties_UB;
			UniformBuffer*		m_pMaterialNumericalProperties_UB;

			Texture2D*			m_pColorOutput;
			Texture2D*			m_pDepthOutput;
		};
		std::vector<FrameResources> m_frameResources;

		RenderPassObject* m_pRenderPassObject;
	};
}