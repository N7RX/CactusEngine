#pragma once
#include "BaseRenderer.h"
#include "RenderTexture.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	// High-performance forward renderer; dedicated for Vulkan device
	class HPForwardRenderer : public BaseRenderer, std::enable_shared_from_this<HPForwardRenderer>
	{
	public:
		HPForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~HPForwardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;

		const std::shared_ptr<GraphicsPipelineObject> GetGraphicsPipeline(EBuiltInShaderProgramType shaderType, const char* passName) const;

	private:
		void BuildRenderResources();

		void CreateFrameResources();
		void TransitionResourceLayouts();
		void CreateRenderPassObjects();
		void CreatePipelineObjects();

		void BuildShadowMapPass();
		void BuildNormalOnlyPass();
		void BuildOpaquePass();
		void BuildGaussianBlurPass();
		void BuildLineDrawingPass();
		void BuildTransparentPass();
		void BuildOpaqueTranspBlendPass();
		void BuildDepthOfFieldPass();

		void CreateLineDrawingMatrices();

	private:
		// Graphics pipelines are organized by ShaderProgram-PassName identifier
		typedef std::unordered_map<const char*, std::shared_ptr<GraphicsPipelineObject>> PassGraphicsPipeline;
		std::unordered_map<EBuiltInShaderProgramType, PassGraphicsPipeline> m_graphicsPipelines;

		std::shared_ptr<FrameBuffer>		m_pShadowMapPassFrameBuffer;
		std::shared_ptr<Texture2D>			m_pShadowMapPassDepthOutput;
		std::shared_ptr<RenderPassObject>	m_pShadowMapRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pNormalOnlyPassFrameBuffer;
		std::shared_ptr<Texture2D>			m_pNormalOnlyPassNormalOutput;
		std::shared_ptr<Texture2D>			m_pNormalOnlyPassPositionOutput;
		std::shared_ptr<Texture2D>			m_pNormalOnlyPassDepthOutput;
		std::shared_ptr<RenderPassObject>	m_pNormalOnlyRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pOpaquePassFrameBuffer;
		std::shared_ptr<Texture2D>			m_pOpaquePassColorOutput;
		std::shared_ptr<Texture2D>			m_pOpaquePassShadowOutput;
		std::shared_ptr<Texture2D>			m_pOpaquePassDepthOutput;
		std::shared_ptr<RenderPassObject>	m_pOpaqueRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pBlurPassFrameBuffer_Horizontal;
		std::shared_ptr<FrameBuffer>		m_pBlurPassFrameBuffer_FinalColor;
		std::shared_ptr<Texture2D>			m_pBlurPassHorizontalColorOutput;
		std::shared_ptr<Texture2D>			m_pBlurPassFinalColorOutput;
		std::shared_ptr<RenderPassObject>	m_pBlurRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pLineDrawingPassFrameBuffer_Curvature;
		std::shared_ptr<FrameBuffer>		m_pLineDrawingPassFrameBuffer_RidgeSearch;
		std::shared_ptr<FrameBuffer>		m_pLineDrawingPassFrameBuffer_Blur_Horizontal;
		std::shared_ptr<FrameBuffer>		m_pLineDrawingPassFrameBuffer_Blur_FinalColor;
		std::shared_ptr<FrameBuffer>		m_pLineDrawingPassFrameBuffer_FinalBlend;
		std::shared_ptr<Texture2D>			m_pLineDrawingPassCurvatureOutput;
		std::shared_ptr<Texture2D>			m_pLineDrawingPassBlurredOutput;
		std::shared_ptr<Texture2D>			m_pLineDrawingPassColorOutput;
		std::shared_ptr<RenderTexture>		m_pLineDrawingMatrixTexture;
		std::shared_ptr<RenderPassObject>	m_pLineDrawingRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pTranspPassFrameBuffer;
		std::shared_ptr<Texture2D>			m_pTranspPassColorOutput;
		std::shared_ptr<Texture2D>			m_pTranspPassDepthOutput;
		std::shared_ptr<RenderPassObject>	m_pTranspRenderPassObject;

		std::shared_ptr<FrameBuffer>		m_pBlendPassFrameBuffer;
		std::shared_ptr<Texture2D>			m_pBlendPassColorOutput;
		std::shared_ptr<RenderPassObject>	m_pBlendRenderPassObject;

		std::shared_ptr<FrameBuffer>			m_pDOFPassFrameBuffer_Horizontal;
		std::shared_ptr<SwapchainFrameBuffers>	m_pDOFPassPresentFrameBuffers;
		std::shared_ptr<Texture2D>				m_pDOFPassHorizontalOutput;
		std::shared_ptr<RenderPassObject>		m_pDOFRenderPassObject;

		std::shared_ptr<Texture2D>		m_pBrushMaskImageTexture_1;
		std::shared_ptr<Texture2D>		m_pBrushMaskImageTexture_2;
		std::shared_ptr<Texture2D>		m_pPencilMaskImageTexture_1;
		std::shared_ptr<Texture2D>		m_pPencilMaskImageTexture_2;
	};

	namespace HPForwardGraphRes
	{
		static const char* SHADOWMAP_PASS_NAME = "ShadowMapPass";
		static const char* NORMAL_ONLY_PASS_NAME = "NormalOnlyPass";
		static const char* OPAQUE_PASS_NAME = "OpaquePass";
		static const char* BLUR_PASS_NAME = "BlurPass";
		static const char* LINEDRAWING_PASS_NAME = "LineDrawingPass";
		static const char* TRANSPARENT_PASS_NAME = "TransparentPass";
		static const char* BLEND_PASS_NAME = "BlendPass";
		static const char* DOF_PASS_NAME = "DOFPass";

		static const char* SHADOWMAP_NODE = "ShadowMapNode";
		static const char* NORMALONLY_NODE = "NormalOnlyNode";
		static const char* OPAQUE_NODE = "OpaqueNode";
		static const char* BLUR_NODE = "BlurNode";
		static const char* LINEDRAWING_NODE = "LineDrawingNode";
		static const char* TRANSP_NODE = "TranspNode";
		static const char* COLORBLEND_D2_NODE = "ColorBlend_DepthBased_2";
		static const char* DOF_NODE = "DOFNode";

		static const char* SHADOWMAP_FB = "ShadowMapFrameBuffer";
		static const char* SHADOWMAP_DEPTH = "ShadowMapDepth";
		static const char* SHADOWMAP_RPO = "ShadowMapRPO"; // RPO stands for render pass object

		static const char* NORMALONLY_FB = "NormalOnlyFrameBuffer";
		static const char* NORMALONLY_NORMAL = "NormalOnlyNormal";
		static const char* NORMALONLY_POSITION = "NormalOnlyPosition";
		static const char* NORMALONLY_RPO = "NormalOnlyRPO";

		static const char* OPAQUE_FB = "OpaqueFrameBuffer";
		static const char* OPAQUE_COLOR = "OpaqueColor";
		static const char* OPAQUE_SHADOW = "OpaqueShadow";
		static const char* OPAQUE_DEPTH = "OpaqueDepth";
		static const char* OPAQUE_RPO = "OpaqueRPO";

		static const char* BLUR_HORI_FB = "BlurFrameBuffer_Horizontal";
		static const char* BLUR_FIN_FB = "BlurFrameBuffer_FinalColor";
		static const char* BLUR_HORIZONTAL_COLOR = "BlurHorizontalColor";
		static const char* BLUR_FINAL_COLOR = "BlurFinalColor";
		static const char* BLUR_RPO = "BlurRPO";

		static const char* LINEDRAWING_CURV_FB = "LineDrawingFrameBuffer_Curvature";
		static const char* LINEDRAWING_RIDG_FB = "LineDrawingFrameBuffer_RidgeSearch";
		static const char* LINEDRAWING_BLUR_HORI_FB = "LineDrawingFrameBuffer_Blur_Horizontal";
		static const char* LINEDRAWING_BLUR_FIN_FB = "LineDrawingFrameBuffer_Blur_FinalColor";
		static const char* LINEDRAWING_FIN_FB = "LineDrawingFrameBuffer_FinalBlend";
		static const char* LINEDRAWING_MATRIX_TEXTURE = "LineDrawMatrixTexture";
		static const char* LINEDRAWING_CURVATURE = "LineDrawingCurvature";
		static const char* LINEDRAWING_BLURRED = "LineDrawingBlurred";
		static const char* LINEDRAWING_COLOR = "LineDrawingColor";
		static const char* LINEDRAWING_RPO = "LineDrawingRPO";

		static const char* TRANSP_FB = "TranspFrameBuffer";
		static const char* TRANSP_COLOR = "TranspColor";
		static const char* TRANSP_DEPTH = "TranspDepth";
		static const char* TRANSP_RPO = "TranspRPO";

		static const char* BLEND_FB = "BlendFrameBuffer";
		static const char* BLEND_COLOR = "BlendColor";
		static const char* BLEND_RPO = "BlendRPO";

		static const char* DOF_HORI_FB = "DOFFrameBuffer_Horizontal";
		static const char* DOF_FIN_FB = "DOFFrameBuffer_Final";
		static const char* DOF_HORIZONTAL = "DOFHorizontal";
		static const char* DOF_RPO = "DOFRPO";

		static const char* BRUSH_MASK_TEXTURE_1 = "BrushMask_1";
		static const char* BRUSH_MASK_TEXTURE_2 = "BrushMask_2";
		static const char* PENCIL_MASK_TEXTURE_1 = "PencilMask_1";
		static const char* PENCIL_MASK_TEXTURE_2 = "PencilMask_2";
	}
}