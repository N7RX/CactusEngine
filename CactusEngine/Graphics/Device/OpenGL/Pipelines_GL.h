#pragma once
#include "GraphicsResources.h"
#include "GraphicsHardwareInterface_GL.h"

namespace Engine
{
	class ShaderProgram_GL;
	class PipelineViewportState_GL;

	// OpenGL render pass object is simply an attachment clear state record
	class RenderPass_GL : public RenderPassObject
	{
	public:
		RenderPass_GL();
		~RenderPass_GL() = default;
		void Initialize();

		bool   m_clearColorOnLoad;
		bool   m_clearDepthOnLoad;
		Color4 m_clearColor;
	};

	struct GraphicsPipelineCreateInfo_GL
	{
		EAssemblyTopology	topologyMode;
		bool				enablePrimitiveRestart;

		bool				enableBlend;
		EBlendFactor		blendSrcFactor;
		EBlendFactor		blendDstFactor;

		bool				enableCulling;
		ECullMode			cullMode;
		EPolygonMode		polygonMode;

		bool				enableDepthTest;
		bool				enableDepthMask;

		bool				enableMultisampling;

		PipelineViewportState_GL* pViewportState;
	};

	// OpenGL graphics pipeline object is simply abstracted as a state record
	class GraphicsPipeline_GL : public GraphicsPipelineObject
	{
	public:
		GraphicsPipeline_GL(GraphicsHardwareInterface_GL* pDevice, ShaderProgram_GL* pShaderProgram, GraphicsPipelineCreateInfo_GL& createInfo);
		~GraphicsPipeline_GL() = default;

		void UpdateViewportState(const PipelineViewportState* pNewState) override;

		void Apply() const;

	private:
		GraphicsHardwareInterface_GL* m_pDevice;
		ShaderProgram_GL* m_pShaderProgram;

		GLenum m_primitiveTopologyMode;
		bool   m_enablePrimitiveRestart;

		bool   m_enableBlend;
		GLenum m_blendSrcFactor;
		GLenum m_blendDstFactor;

		bool   m_enableCulling;
		GLenum m_cullMode;
		GLenum m_polygonMode;

		bool   m_enableDepthTest;
		bool   m_enableDepthMask;

		bool   m_enableMultisampling;

		PipelineViewportState_GL* m_pViewportState;
	};

	class PipelineInputAssemblyState_GL : public PipelineInputAssemblyState
	{
	public:
		PipelineInputAssemblyState_GL(const PipelineInputAssemblyStateCreateInfo& createInfo);

		EAssemblyTopology topologyMode;
		bool enablePrimitiveRestart;
	};

	class PipelineColorBlendState_GL : public PipelineColorBlendState
	{
	public:
		PipelineColorBlendState_GL(const PipelineColorBlendStateCreateInfo& createInfo);

		bool enableBlend;
		EBlendFactor blendSrcFactor;
		EBlendFactor blendDstFactor;
	};

	class PipelineRasterizationState_GL : public PipelineRasterizationState
	{
	public:
		PipelineRasterizationState_GL(const PipelineRasterizationStateCreateInfo& createInfo);

		bool enableCull;
		ECullMode cullMode;
		EPolygonMode polygonMode;
	};

	class PipelineDepthStencilState_GL : public PipelineDepthStencilState
	{
	public:
		PipelineDepthStencilState_GL(const PipelineDepthStencilStateCreateInfo& createInfo);

		bool enableDepthTest;
		bool enableDepthMask;
	};

	class PipelineMultisampleState_GL : public PipelineMultisampleState
	{
	public:
		PipelineMultisampleState_GL(const PipelineMultisampleStateCreateInfo& createInfo);

		bool enableMultisampling;
	};

	class PipelineViewportState_GL : public PipelineViewportState
	{
	public:
		PipelineViewportState_GL(const PipelineViewportStateCreateInfo& createInfo);

		void UpdateResolution(uint32_t width, uint32_t height) override;

		uint32_t viewportWidth;
		uint32_t viewportHeight;
	};
}