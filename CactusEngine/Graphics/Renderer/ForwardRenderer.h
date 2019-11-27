#pragma once
#include "BaseRenderer.h"
#include "RenderTexture.h"

namespace Engine
{
	class ForwardRenderer : public BaseRenderer, std::enable_shared_from_this<ForwardRenderer>
	{
	public:
		ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~ForwardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;

	private:
		void BuildFrameResources();
		void BuildNormalOnlyPass();
		void BuildOpaquePass();
		void BuildGaussianBlurPass();
		void BuildLineDrawingPass();
		void BuildTransparentPass();
		void BuildOpaqueTranspBlendPass();

		void CreateLineDrawingMatrices();

	private:
		std::shared_ptr<FrameBuffer> m_pNormalOnlyPassFrameBuffer;
		std::shared_ptr<Texture2D> m_pNormalOnlyPassColorOutput;
		std::shared_ptr<Texture2D> m_pNormalOnlyPassDepthOutput;

		std::shared_ptr<FrameBuffer> m_pOpaquePassFrameBuffer;
		std::shared_ptr<Texture2D> m_pOpaquePassColorOutput;
		std::shared_ptr<Texture2D> m_pOpaquePassDepthOutput;

		std::shared_ptr<FrameBuffer> m_pBlurPassFrameBuffer;
		std::shared_ptr<Texture2D> m_pBlurPassHorizontalColorOutput;
		std::shared_ptr<Texture2D> m_pBlurPassFinalColorOutput;

		std::shared_ptr<FrameBuffer> m_pLineDrawingPassFrameBuffer;
		std::shared_ptr<Texture2D> m_pLineDrawingPassCurvatureOutput;
		std::shared_ptr<Texture2D> m_pLineDrawingPassBlurredOutput;
		std::shared_ptr<Texture2D> m_pLineDrawingPassColorOutput;
		std::shared_ptr<RenderTexture> m_pLineDrawingMatrixTexture;

		std::shared_ptr<FrameBuffer> m_pTranspPassFrameBuffer;
		std::shared_ptr<Texture2D> m_pTranspPassColorOutput;
		std::shared_ptr<Texture2D> m_pTranspPassDepthOutput;
	};

	namespace ForwardGraphRes
	{
		static const char* NORMALONLY_NODE = "NormalOnlyNode";
		static const char* OPAQUE_NODE = "OpaqueNode";
		static const char* BLUR_NODE = "BlurNode";
		static const char* LINEDRAWING_NODE = "LineDrawingNode";
		static const char* TRANSP_NODE = "TranspNode";
		static const char* COLORBLEND_D2_NODE = "ColorBlend_DepthBased_2";		

		static const char* NORMALONLY_FB = "NormalOnlyFrameBuffer";
		static const char* NORMALONLY_COLOR = "NormalOnlyColor";

		static const char* OPAQUE_FB = "OpaqueFrameBuffer";
		static const char* OPAQUE_COLOR = "OpaqueColor";
		static const char* OPAQUE_DEPTH = "OpaqueDepth";

		static const char* BLUR_FB = "BlurFrameBuffer";
		static const char* BLUR_HORIZONTAL_COLOR = "BlurHorizontalColor";
		static const char* BLUR_FINAL_COLOR = "BlurFinalColor";

		static const char* LINEDRAWING_FB = "LineDrawingFrameBuffer";
		static const char* LINEDRAWING_MATRIX_TEXTURE = "LineDrawMatrixTexture";
		static const char* LINEDRAWING_CURVATURE = "LineDrawingCurvature";
		static const char* LINEDRAWING_BLURRED = "LineDrawingBlurred";
		static const char* LINEDRAWING_COLOR = "LineDrawingColor";

		static const char* TRANSP_FB = "TranspFrameBuffer";
		static const char* TRANSP_COLOR = "TranspColor";
		static const char* TRANSP_DEPTH = "TranspDepth";
	}
}