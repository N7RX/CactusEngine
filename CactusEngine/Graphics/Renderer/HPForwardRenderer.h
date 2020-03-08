#pragma once
#include "BaseRenderer.h"
#include "RenderTexture.h"
#include "BuiltInShaderType.h"
#include "CommandResources.h"
#include "SafeBasicTypes.h"

namespace Engine
{
	// High-performance forward renderer; dedicated for Vulkan-like device
	class HPForwardRenderer : public BaseRenderer, std::enable_shared_from_this<HPForwardRenderer>
	{
	public:
		HPForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~HPForwardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;

		const std::shared_ptr<GraphicsPipelineObject> GetGraphicsPipeline(EBuiltInShaderProgramType shaderType, const char* passName) const;

		void WriteCommandRecordList(const char* pNodeName, const std::shared_ptr<DrawingCommandBuffer>& pCommandBuffer);

	private:
		void BuildRenderResources();

		void CreateFrameTextures();
		void TransitionImageLayouts();
		void CreateRenderPassObjects();
		void CreateFrameBuffers();
		void CreateUniformBuffers();
		void CreatePipelineObjects();

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

		void ResetUniformBufferSubAllocation();

	private:

		// Uniform buffers use subresgions from a large buffer

		const uint32_t MAX_UNIFORM_SUB_ALLOCATION = 1024;
		const uint32_t MIN_UNIFORM_SUB_ALLOCATION = 32;

		// Graphics pipelines are organized by ShaderProgram-PassName identifier

		typedef std::unordered_map<const char*, std::shared_ptr<GraphicsPipelineObject>> PassGraphicsPipeline;
		std::unordered_map<EBuiltInShaderProgramType, PassGraphicsPipeline> m_graphicsPipelines;

		// Command record list

		std::unordered_map<uint32_t, std::shared_ptr<DrawingCommandBuffer>> m_commandRecordReadyList; // Submit Priority - Recorded Command Buffer
		std::unordered_map<const char*, uint32_t> m_renderNodePriorities; // Render Node Name - Submit Priority

		std::mutex m_commandRecordListMutex;
		std::condition_variable m_commandRecordListCv;
		SafeBool m_newCommandRecorded;

		// Pass organized resources

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

		std::shared_ptr<FrameBuffer>		m_pDOFPassFrameBuffer_Horizontal;
		std::shared_ptr<Texture2D>			m_pDOFPassHorizontalOutput;
		std::shared_ptr<RenderPassObject>	m_pDOFRenderPassObject;

		std::shared_ptr<SwapchainFrameBuffers> m_pPresentFrameBuffers;
		std::shared_ptr<RenderPassObject>	m_pPresentRenderPassObject;

		// Extra input resources

		std::shared_ptr<Texture2D>			m_pBrushMaskImageTexture_1;
		std::shared_ptr<Texture2D>			m_pBrushMaskImageTexture_2;
		std::shared_ptr<Texture2D>			m_pPencilMaskImageTexture_1;
		std::shared_ptr<Texture2D>			m_pPencilMaskImageTexture_2;

		// Uniform buffers

		std::shared_ptr<UniformBuffer>		m_pTransformMatrices_UB;
		std::shared_ptr<UniformBuffer>		m_pLightSpaceTransformMatrix_UB;
		std::shared_ptr<UniformBuffer>		m_pMaterialNumericalProperties_UB;
		std::shared_ptr<UniformBuffer>		m_pCameraPropertie_UB;
		std::shared_ptr<UniformBuffer>		m_pSystemVariables_UB;
		std::shared_ptr<UniformBuffer>		m_pControlVariables_UB;
	};

	namespace HPForwardGraphRes
	{
		static const char* PASSNAME_SHADOWMAP = "ShadowMapPass";
		static const char* PASSNAME_NORMAL_ONLY = "NormalOnlyPass";
		static const char* PASSNAME_OPAQUE = "OpaquePass";
		static const char* PASSNAME_BLUR = "BlurPass";
		static const char* PASSNAME_LINEDRAWING = "LineDrawingPass";
		static const char* PASSNAME_TRANSPARENT = "TransparentPass";
		static const char* PASSNAME_BLEND = "BlendPass";
		static const char* PASSNAME_DOF = "DOFPass";
		static const char* PASSNAME_PRESENT = "PresentPass";

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
		static const char* RPO_SHADOWMAP = "ShadowMapRPO";

		static const char* FB_NORMALONLY = "NormalOnlyFrameBuffer";
		static const char* TX_NORMALONLY_NORMAL = "NormalOnlyNormal";
		static const char* TX_NORMALONLY_POSITION = "NormalOnlyPosition";
		static const char* RPO_NORMALONLY = "NormalOnlyRPO";

		static const char* FB_OPAQUE = "OpaqueFrameBuffer";
		static const char* TX_OPAQUE_COLOR = "OpaqueColor";
		static const char* TX_OPAQUE_SHADOW = "OpaqueShadow";
		static const char* TX_OPAQUE_DEPTH = "OpaqueDepth";
		static const char* RPO_OPAQUE = "OpaqueRPO";

		static const char* FB_BLUR_HORI = "BlurFrameBuffer_Horizontal";
		static const char* FB_BLUR_FIN = "BlurFrameBuffer_FinalColor";
		static const char* TX_BLUR_HORIZONTAL_COLOR = "BlurHorizontalColor";
		static const char* TX_BLUR_FINAL_COLOR = "BlurFinalColor";
		static const char* RPO_BLUR = "BlurRPO";

		static const char* FB_LINEDRAWING_CURV = "LineDrawingFrameBuffer_Curvature";
		static const char* FB_LINEDRAWING_RIDG = "LineDrawingFrameBuffer_RidgeSearch";
		static const char* FB_LINEDRAWING_BLUR_HORI = "LineDrawingFrameBuffer_Blur_Horizontal";
		static const char* FB_LINEDRAWING_BLUR_FIN = "LineDrawingFrameBuffer_Blur_FinalColor";
		static const char* FB_LINEDRAWING_FIN = "LineDrawingFrameBuffer_FinalBlend";
		static const char* TX_LINEDRAWING_MATRIX_TEXTURE = "LineDrawMatrixTexture";
		static const char* TX_LINEDRAWING_CURVATURE = "LineDrawingCurvature";
		static const char* TX_LINEDRAWING_BLURRED = "LineDrawingBlurred";
		static const char* TX_LINEDRAWING_COLOR = "LineDrawingColor";
		static const char* RPO_LINEDRAWING = "LineDrawingRPO";

		static const char* FB_TRANSP = "TranspFrameBuffer";
		static const char* TX_TRANSP_COLOR = "TranspColor";
		static const char* TX_TRANSP_DEPTH = "TranspDepth";
		static const char* RPO_TRANSP = "TranspRPO";

		static const char* FB_BLEND = "BlendFrameBuffer";
		static const char* TX_BLEND_COLOR = "BlendColor";
		static const char* TX_BLEND_RPO = "BlendRPO";

		static const char* FB_DOF_HORI = "DOFFrameBuffer_Horizontal";	
		static const char* TX_DOF_HORIZONTAL = "DOFHorizontal";
		static const char* RPO_DOF = "DOFRPO";

		static const char* FB_PRESENT = "PresentFrameBuffer";
		static const char* RPO_PRESENT = "PresentFrameRPO";

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