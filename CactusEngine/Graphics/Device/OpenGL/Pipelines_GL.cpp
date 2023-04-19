#include "Pipelines_GL.h"
#include "GHIUtilities_GL.h"
#include "Shaders_GL.h"

namespace Engine
{
	RenderPass_GL::RenderPass_GL()
		: m_clearColorOnLoad(false),
		m_clearDepthOnLoad(false),
		m_clearColor(Color4(1))
	{

	}

	void RenderPass_GL::Initialize()
	{
		if (m_clearColorOnLoad)
		{
			glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
		}

		if (m_clearColorOnLoad && m_clearDepthOnLoad)
		{
			GLboolean depthMaskState;
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskState);
			if (!depthMaskState)
			{
				glDepthMask(GL_TRUE);
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (!depthMaskState)
			{
				glDepthMask(GL_FALSE);
			}
		}
		else if (m_clearColorOnLoad)
		{
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else if (m_clearDepthOnLoad)
		{
			GLboolean depthMaskState;
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskState);
			if (!depthMaskState)
			{
				glDepthMask(GL_TRUE);
			}

			glClear(GL_DEPTH_BUFFER_BIT);

			if (!depthMaskState)
			{
				glDepthMask(GL_FALSE);
			}
		}
	}

	GraphicsPipeline_GL::GraphicsPipeline_GL(GraphicsHardwareInterface_GL* pDevice, ShaderProgram_GL* pShaderProgram, GraphicsPipelineCreateInfo_GL& createInfo)
		: m_pDevice(pDevice),
		m_pShaderProgram(pShaderProgram),
		m_primitiveTopologyMode(OpenGLAssemblyTopologyMode(createInfo.topologyMode)),
		m_enablePrimitiveRestart(createInfo.enablePrimitiveRestart),
		m_enableBlend(createInfo.enableBlend),
		m_blendSrcFactor(0),
		m_blendDstFactor(0),
		m_enableCulling(createInfo.enableCulling),
		m_cullMode(0),
		m_polygonMode(OpenGLPolygonMode(createInfo.polygonMode)),
		m_enableDepthTest(createInfo.enableDepthTest),
		m_enableDepthMask(createInfo.enableDepthMask),
		m_enableMultisampling(createInfo.enableMultisampling),
		m_pViewportState(createInfo.pViewportState)
	{
		if (m_enableBlend)
		{
			m_blendSrcFactor = OpenGLBlendFactor(createInfo.blendSrcFactor);
			m_blendDstFactor = OpenGLBlendFactor(createInfo.blendDstFactor);
		}

		if (m_enableCulling)
		{
			m_cullMode = OpenGLCullMode(createInfo.cullMode);
		}
	}

	void GraphicsPipeline_GL::UpdateViewportState(const PipelineViewportState* pNewState)
	{
		m_pViewportState = (PipelineViewportState_GL*)pNewState;
	}

	void GraphicsPipeline_GL::Apply() const
	{
		glUseProgram(m_pShaderProgram->GetGLProgramID());

		m_pDevice->SetPrimitiveTopology(m_primitiveTopologyMode);
		if (m_enablePrimitiveRestart)
		{
			glEnable(GL_PRIMITIVE_RESTART);
		}
		else
		{
			glDisable(GL_PRIMITIVE_RESTART);
		}

		if (m_enableBlend)
		{
			glEnable(GL_BLEND);
			glBlendFunc(m_blendSrcFactor, m_blendDstFactor);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		if (m_enableCulling)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(m_cullMode);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
		glPolygonMode(GL_FRONT_AND_BACK, m_polygonMode);

		if (m_enableDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		glDepthMask(m_enableDepthMask ? GL_TRUE : GL_FALSE);

		if (m_enableMultisampling)
		{
			glEnable(GL_MULTISAMPLE);
		}
		else
		{
			glDisable(GL_MULTISAMPLE);
		}

		glViewport(0, 0, m_pViewportState->viewportWidth, m_pViewportState->viewportHeight);
	}

	PipelineInputAssemblyState_GL::PipelineInputAssemblyState_GL(const PipelineInputAssemblyStateCreateInfo& createInfo)
	{
		topologyMode = createInfo.topology;
		enablePrimitiveRestart = createInfo.enablePrimitiveRestart;
	}

	PipelineColorBlendState_GL::PipelineColorBlendState_GL(const PipelineColorBlendStateCreateInfo& createInfo)
		: enableBlend(false),
		blendSrcFactor(EBlendFactor::SrcAlpha),
		blendDstFactor(EBlendFactor::OneMinusSrcAlpha)
	{
		if (createInfo.blendStateDescs.size() == 0)
		{
			enableBlend = false;
			return;
		}

		// For OpenGL, take the first desc as blend state configuration

		enableBlend = createInfo.blendStateDescs[0].enableBlend;
		blendSrcFactor = createInfo.blendStateDescs[0].srcColorBlendFactor;
		blendDstFactor = createInfo.blendStateDescs[0].dstColorBlendFactor;
	}

	PipelineRasterizationState_GL::PipelineRasterizationState_GL(const PipelineRasterizationStateCreateInfo& createInfo)
		: enableCull(createInfo.cullMode == ECullMode::None ? false : true),
		cullMode(createInfo.cullMode),
		polygonMode(createInfo.polygonMode)
	{

	}

	PipelineDepthStencilState_GL::PipelineDepthStencilState_GL(const PipelineDepthStencilStateCreateInfo& createInfo)
		: enableDepthTest(createInfo.enableDepthTest),
		enableDepthMask(createInfo.enableDepthWrite)
	{

	}

	PipelineMultisampleState_GL::PipelineMultisampleState_GL(const PipelineMultisampleStateCreateInfo& createInfo)
		: enableMultisampling(createInfo.enableSampleShading)
	{
		
	}

	PipelineViewportState_GL::PipelineViewportState_GL(const PipelineViewportStateCreateInfo& createInfo)
		: viewportWidth(createInfo.width),
		viewportHeight(createInfo.height)
	{

	}

	void PipelineViewportState_GL::UpdateResolution(uint32_t width, uint32_t height)
	{
		viewportWidth = width;
		viewportHeight = height;
	}
}