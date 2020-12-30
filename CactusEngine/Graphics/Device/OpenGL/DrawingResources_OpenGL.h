#pragma once
#include "DrawingResources.h"
#include "DrawingDevice_OpenGL.h"

namespace Engine
{
	class VertexBuffer_OpenGL : public VertexBuffer
	{
	public:
		VertexBuffer_OpenGL();
		~VertexBuffer_OpenGL();

	public:
		unsigned int m_vao;
		unsigned int m_vboIndices;
		unsigned int m_vboVertices;
	};

	class Texture2D_OpenGL : public Texture2D
	{
	public:
		Texture2D_OpenGL();
		~Texture2D_OpenGL();

		GLuint GetGLTextureID() const;
		void SetGLTextureID(GLuint id);

		void MarkTextureSize(uint32_t width, uint32_t height);

		// Not used in OpenGL device
		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	private:
		GLuint m_glTextureID;
	};

	class UniformBuffer_OpenGL : public UniformBuffer
	{
	public:
		~UniformBuffer_OpenGL();

		void SetGLBufferID(GLuint id);
		GLuint GetGLBufferID() const;

		void UpdateBufferData(const void* pData) override;
		void UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size) override;
		std::shared_ptr<SubUniformBuffer> AllocateSubBuffer(uint32_t size) override;
		void ResetSubBufferAllocation() override;

	private:
		GLuint m_glBufferID = -1;
	};

	// OpenGL render pass object is simply an attachment clear state record
	class RenderPass_OpenGL : public RenderPassObject
	{
	public:
		RenderPass_OpenGL();
		~RenderPass_OpenGL() = default;
		void Initialize();

		bool   m_clearColorOnLoad;
		bool   m_clearDepthOnLoad;
		Color4 m_clearColor;
	};

	class FrameBuffer_OpenGL : public FrameBuffer
	{
	public:
		~FrameBuffer_OpenGL();

		GLuint GetGLFrameBufferID() const;
		uint32_t GetFrameBufferID() const override;

		void SetGLFrameBufferID(GLuint id);
		void MarkFrameBufferSize(uint32_t width, uint32_t height);

		void AddColorAttachment(GLenum attachment);
		size_t GetColorAttachmentCount() const;
		GLenum GetColorAttachment(uint32_t index) const;
		const GLenum* GetColorAttachments() const;

	private:
		GLuint m_glFrameBufferID = -1;
		std::vector<GLenum> m_bufferAttachments;
	};

	class VertexShader_OpenGL : public VertexShader
	{
	public:
		VertexShader_OpenGL(const char* filePath);
		~VertexShader_OpenGL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class FragmentShader_OpenGL : public FragmentShader
	{
	public:
		FragmentShader_OpenGL(const char* filePath);
		~FragmentShader_OpenGL() = default;

		GLuint GetGLShaderID() const;

	private:
		GLuint m_glShaderID;
	};

	class ShaderProgram_OpenGL : public ShaderProgram
	{
	public:
		ShaderProgram_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader);

		GLuint GetGLProgramID() const;

		unsigned int GetParamBinding(const char* paramName) const override;
		void Reset() override;

		void UpdateParameterValue(unsigned int binding, EDescriptorType type, const std::shared_ptr<RawResource> pRes);

	private:
		void ReflectParamLocations();

	private:
		GLuint m_glProgramID;
		std::unordered_map<const char*, unsigned int> m_paramBindings;
	};

	struct GraphicsPipelineCreateInfo_OpenGL
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

		uint32_t			viewportWidth;
		uint32_t			viewportHeight;
	};

	// OpenGL graphics pipeline object is simply abstracted as a state record
	class GraphicsPipeline_OpenGL : public GraphicsPipelineObject
	{
	public:
		GraphicsPipeline_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<ShaderProgram_OpenGL> pShaderProgram, GraphicsPipelineCreateInfo_OpenGL& createInfo);
		~GraphicsPipeline_OpenGL() = default;

		void Apply() const;

	private:
		DrawingDevice_OpenGL* m_pDevice;
		std::shared_ptr<ShaderProgram_OpenGL> m_pShaderProgram;

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

		uint32_t m_viewportWidth;
		uint32_t m_viewportHeight;
	};

	class PipelineInputAssemblyState_OpenGL : public PipelineInputAssemblyState
	{
	public:
		PipelineInputAssemblyState_OpenGL(const PipelineInputAssemblyStateCreateInfo& createInfo);

		EAssemblyTopology topologyMode;
		bool enablePrimitiveRestart;
	};

	class PipelineColorBlendState_OpenGL : public PipelineColorBlendState
	{
	public:
		PipelineColorBlendState_OpenGL(const PipelineColorBlendStateCreateInfo& createInfo);

		bool enableBlend;
		EBlendFactor blendSrcFactor;
		EBlendFactor blendDstFactor;
	};

	class PipelineRasterizationState_OpenGL : public PipelineRasterizationState
	{
	public:
		PipelineRasterizationState_OpenGL(const PipelineRasterizationStateCreateInfo& createInfo);

		bool enableCull;
		ECullMode cullMode;
		EPolygonMode polygonMode;
	};

	class PipelineDepthStencilState_OpenGL : public PipelineDepthStencilState
	{
	public:
		PipelineDepthStencilState_OpenGL(const PipelineDepthStencilStateCreateInfo& createInfo);

		bool enableDepthTest;
		bool enableDepthMask;
	};

	class PipelineMultisampleState_OpenGL : public PipelineMultisampleState
	{
	public:
		PipelineMultisampleState_OpenGL(const PipelineMultisampleStateCreateInfo& createInfo);

		bool enableMultisampling;
	};

	class PipelineViewportState_OpenGL : public PipelineViewportState
	{
	public:
		PipelineViewportState_OpenGL(const PipelineViewportStateCreateInfo& createInfo);

		unsigned int viewportWidth;
		unsigned int viewportHeight;
	};
}