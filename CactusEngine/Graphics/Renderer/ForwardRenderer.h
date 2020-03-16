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

		void CreateFrameTextures();
		void CreateFrameBuffers();
		void CreateUniformBuffers();

		void BuildShadowMapPass();
		void BuildNormalOnlyPass();
		void BuildOpaquePass();
		void BuildGaussianBlurPass();
		void BuildLineDrawingPass();
		void BuildTransparentPass();
		void BuildOpaqueTranspBlendPass();
		void BuildDepthOfFieldPass();

		void BuildRenderNodeDependencies();

		void CreateLineDrawingMatrices();

	private:

		const uint32_t SHADOW_MAP_RESOLUTION = 4096;

		std::shared_ptr<FrameBuffer>	m_pShadowMapPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pShadowMapPassDepthOutput;

		std::shared_ptr<FrameBuffer>	m_pNormalOnlyPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pNormalOnlyPassNormalOutput;
		std::shared_ptr<Texture2D>		m_pNormalOnlyPassPositionOutput;
		std::shared_ptr<Texture2D>		m_pNormalOnlyPassDepthOutput;

		std::shared_ptr<FrameBuffer>	m_pOpaquePassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pOpaquePassColorOutput;
		std::shared_ptr<Texture2D>		m_pOpaquePassShadowOutput;
		std::shared_ptr<Texture2D>		m_pOpaquePassDepthOutput;

		std::shared_ptr<FrameBuffer>	m_pBlurPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pBlurPassHorizontalColorOutput;
		std::shared_ptr<Texture2D>		m_pBlurPassFinalColorOutput;

		std::shared_ptr<FrameBuffer>	m_pLineDrawingPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pLineDrawingPassCurvatureOutput;
		std::shared_ptr<Texture2D>		m_pLineDrawingPassBlurredOutput;
		std::shared_ptr<Texture2D>		m_pLineDrawingPassColorOutput;
		std::shared_ptr<RenderTexture>	m_pLineDrawingMatrixTexture;

		std::shared_ptr<FrameBuffer>	m_pTranspPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pTranspPassColorOutput;
		std::shared_ptr<Texture2D>		m_pTranspPassDepthOutput;

		std::shared_ptr<FrameBuffer>	m_pBlendPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pBlendPassColorOutput;

		std::shared_ptr<FrameBuffer>	m_pDOFPassFrameBuffer;
		std::shared_ptr<Texture2D>		m_pDOFPassHorizontalOutput;

		std::shared_ptr<Texture2D>		m_pBrushMaskImageTexture_1;
		std::shared_ptr<Texture2D>		m_pBrushMaskImageTexture_2;
		std::shared_ptr<Texture2D>		m_pPencilMaskImageTexture_1;
		std::shared_ptr<Texture2D>		m_pPencilMaskImageTexture_2;

		std::shared_ptr<UniformBuffer>	m_pTransformMatrices_UB;
		std::shared_ptr<UniformBuffer>	m_pLightSpaceTransformMatrix_UB;
		std::shared_ptr<UniformBuffer>	m_pMaterialNumericalProperties_UB;
		std::shared_ptr<UniformBuffer>	m_pCameraPropertie_UB;
		std::shared_ptr<UniformBuffer>	m_pSystemVariables_UB;
		std::shared_ptr<UniformBuffer>	m_pControlVariables_UB;
	};

	namespace ForwardGraphRes
	{
		static const char* NODE_SHADOWMAP = "ShadowMapNode";
		static const char* NODE_NORMALONLY = "NormalOnlyNode";
		static const char* NODE_OPAQUE = "OpaqueNode";
		static const char* NODE_BLUR = "BlurNode";
		static const char* NODE_LINEDRAWING = "LineDrawingNode";
		static const char* NODE_TRANSP = "TranspNode";
		static const char* NODE_COLORBLEND_D2 = "ColorBlend_DepthBased_2";
		static const char* NODE_DOF = "DOFNode";

		static const char* FB_SHADOWMAP = "ShadowMapFrameBuffer";
		static const char* TX_SHADOWMAP_DEPTH = "ShadowMapDepth";

		static const char* FB_NORMALONLY = "NormalOnlyFrameBuffer";
		static const char* TX_NORMALONLY_NORMAL = "NormalOnlyNormal";
		static const char* TX_NORMALONLY_POSITION = "NormalOnlPosition";

		static const char* FB_OPAQUE = "OpaqueFrameBuffer";
		static const char* TX_OPAQUE_COLOR = "OpaqueColor";
		static const char* TX_OPAQUE_SHADOW = "OpaqueShadow";
		static const char* TX_OPAQUE_DEPTH = "OpaqueDepth";

		static const char* FB_BLUR = "BlurFrameBuffer";
		static const char* TX_BLUR_HORIZONTAL_COLOR = "BlurHorizontalColor";
		static const char* TX_BLUR_FINAL_COLOR = "BlurFinalColor";

		static const char* FB_LINEDRAWING = "LineDrawingFrameBuffer";
		static const char* TX_LINEDRAWING_MATRIX_TEXTURE = "LineDrawMatrixTexture";
		static const char* TX_LINEDRAWING_CURVATURE = "LineDrawingCurvature";
		static const char* TX_LINEDRAWING_BLURRED = "LineDrawingBlurred";
		static const char* TX_LINEDRAWING_COLOR = "LineDrawingColor";

		static const char* FB_TRANSP = "TranspFrameBuffer";
		static const char* TX_TRANSP_COLOR = "TranspColor";
		static const char* TX_TRANSP_DEPTH = "TranspDepth";

		static const char* FB_BLEND = "BlendFrameBuffer";
		static const char* TX_BLEND_COLOR = "BlendColor";

		static const char* FB_DOF = "DOFFrameBuffer";
		static const char* TX_DOF_HORIZONTAL = "DOFHorizontal";

		static const char* TX_BRUSH_MASK_TEXTURE_1 = "BrushMask_1";
		static const char* TX_BRUSH_MASK_TEXTURE_2 = "BrushMask_2";
		static const char* TX_PENCIL_MASK_TEXTURE_1 = "PencilMask_1";
		static const char* TX_PENCIL_MASK_TEXTURE_2 = "PencilMask_2";

		static const char* UB_TRANSFORM_MATRICES = "TransformMatrices";
		static const char* UB_LIGHTSPACE_TRANSFORM_MATRIX = "LightSpaceTransformMatrix";
		static const char* UB_MATERIAL_NUMERICAL_PROPERTIES = "MaterialNumericalProperties";
		static const char* UB_CAMERA_PROPERTIES = "CameraProperties";
		static const char* UB_SYSTEM_VARIABLES = "SystemVariables";
		static const char* UB_CONTROL_VARIABLES = "ControlVariables";
	}
}