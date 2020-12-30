#include "DrawingResources_OpenGL.h"
#include "DrawingUtil_OpenGL.h"
#include "BuiltInShaderType.h"
#include "ImageTexture.h"
#include "RenderTexture.h";

using namespace Engine;

VertexBuffer_OpenGL::VertexBuffer_OpenGL()
	: m_vao(-1), m_vboIndices(-1), m_vboVertices(-1)
{
}

VertexBuffer_OpenGL::~VertexBuffer_OpenGL()
{
	glDeleteBuffers(1, &m_vboIndices);
	glDeleteBuffers(1, &m_vboVertices);
	glDeleteVertexArrays(1, &m_vao);
}

Texture2D_OpenGL::Texture2D_OpenGL()
	: Texture2D(ETexture2DSource::RawDeviceTexture), m_glTextureID(-1)
{

}

Texture2D_OpenGL::~Texture2D_OpenGL()
{
	glDeleteTextures(1, &m_glTextureID);
}

GLuint Texture2D_OpenGL::GetGLTextureID() const
{
	return m_glTextureID;
}

void Texture2D_OpenGL::SetGLTextureID(GLuint id)
{
	m_glTextureID = id;
}

void Texture2D_OpenGL::MarkTextureSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

bool Texture2D_OpenGL::HasSampler() const
{
	std::cerr << "OpenGL: shouldn't call HasSampler on OpenGL texture 2D.\n";
	return false;
}

void Texture2D_OpenGL::SetSampler(const std::shared_ptr<TextureSampler> pSampler)
{
	// "OpenGL: shouldn't call SetSampler on OpenGL texture 2D."
}

std::shared_ptr<TextureSampler> Texture2D_OpenGL::GetSampler() const
{
	std::cerr << "OpenGL: shouldn't call GetSampler on OpenGL texture 2D.\n";
	return nullptr;
}

UniformBuffer_OpenGL::~UniformBuffer_OpenGL()
{
	glDeleteBuffers(1, &m_glBufferID);
}

void UniformBuffer_OpenGL::SetGLBufferID(GLuint id)
{
	m_glBufferID = id;
}

GLuint UniformBuffer_OpenGL::GetGLBufferID() const
{
	return m_glBufferID;
}

void UniformBuffer_OpenGL::UpdateBufferData(const void* pData)
{
	assert(m_sizeInBytes != 0);
	assert(m_glBufferID != -1);

	glBindBuffer(GL_UNIFORM_BUFFER, m_glBufferID);
	glBufferData(GL_UNIFORM_BUFFER, m_sizeInBytes, pData, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer_OpenGL::UpdateBufferSubData(const void* pData, uint32_t offset, uint32_t size)
{
	assert(m_sizeInBytes != 0 && size != 0 && offset + size <= m_sizeInBytes);
	assert(m_glBufferID != -1);

	glBindBuffer(GL_UNIFORM_BUFFER, m_glBufferID);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, pData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

std::shared_ptr<SubUniformBuffer> UniformBuffer_OpenGL::AllocateSubBuffer(uint32_t size)
{
	std::cerr << "OpenGL: shouldn't call AllocateSubBuffer on OpenGL uniform buffer.\n";
	return nullptr;
}

void UniformBuffer_OpenGL::ResetSubBufferAllocation()
{
	// "OpenGL: shouldn't call ResetSubBufferAllocation on OpenGL uniform buffer."
}

RenderPass_OpenGL::RenderPass_OpenGL()
	: m_clearColorOnLoad(false), m_clearDepthOnLoad(false), m_clearColor(Color4(1))
{

}

void RenderPass_OpenGL::Initialize()
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

FrameBuffer_OpenGL::~FrameBuffer_OpenGL()
{
	glDeleteFramebuffers(1, &m_glFrameBufferID);
}

GLuint FrameBuffer_OpenGL::GetGLFrameBufferID() const
{
	return m_glFrameBufferID;
}

uint32_t FrameBuffer_OpenGL::GetFrameBufferID() const
{
	return static_cast<uint32_t>(m_glFrameBufferID);
}

void FrameBuffer_OpenGL::SetGLFrameBufferID(GLuint id)
{
	m_glFrameBufferID = id;
}

void FrameBuffer_OpenGL::MarkFrameBufferSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void FrameBuffer_OpenGL::AddColorAttachment(GLenum attachment)
{
	m_bufferAttachments.emplace_back(attachment);
}

size_t FrameBuffer_OpenGL::GetColorAttachmentCount() const
{
	return m_bufferAttachments.size();
}

GLenum FrameBuffer_OpenGL::GetColorAttachment(uint32_t index) const
{
	assert(index < m_bufferAttachments.size());

	return m_bufferAttachments[index];
}

const GLenum* FrameBuffer_OpenGL::GetColorAttachments() const
{
	return m_bufferAttachments.data();
}

VertexShader_OpenGL::VertexShader_OpenGL(const char* filePath)
{
	GLchar* source = ReadSourceFileAsChar(filePath);
	if (source == NULL)
	{
		throw std::runtime_error("OpenGL: Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("OpenGL: Failed to compile shader.");
	}
}

GLuint VertexShader_OpenGL::GetGLShaderID() const
{
	return m_glShaderID;
}

FragmentShader_OpenGL::FragmentShader_OpenGL(const char* filePath)
{
	GLchar* source = ReadSourceFileAsChar(filePath);
	if (source == NULL)
	{
		throw std::runtime_error("OpenGL: Failed to read shader source file.");
	}

	m_glShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_glShaderID, 1, &source, NULL);
	glCompileShader(m_glShaderID);

	GLint compiled;
	glGetShaderiv(m_glShaderID, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderCompileError_GL(m_glShaderID);
		throw std::runtime_error("OpenGL: Failed to compile shader.");
	}
}

GLuint FragmentShader_OpenGL::GetGLShaderID() const
{
	return m_glShaderID;
}

ShaderProgram_OpenGL::ShaderProgram_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<VertexShader_OpenGL> pVertexShader, const std::shared_ptr<FragmentShader_OpenGL> pFragmentShader)
	: ShaderProgram((uint32_t)EShaderType::Vertex | (uint32_t)EShaderType::Fragment)
{
	m_pDevice = pDevice;
	m_glProgramID = glCreateProgram();

	glAttachShader(m_glProgramID, pVertexShader->GetGLShaderID());
	glAttachShader(m_glProgramID, pFragmentShader->GetGLShaderID());

	glLinkProgram(m_glProgramID);

	GLint linked;
	glGetProgramiv(m_glProgramID, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		PrintProgramLinkError_GL(m_glProgramID);
		throw std::runtime_error("OpenGL: Shader program failed to link.");
	}

	ReflectParamLocations();
}

GLuint ShaderProgram_OpenGL::GetGLProgramID() const
{
	return m_glProgramID;
}

unsigned int ShaderProgram_OpenGL::GetParamBinding(const char* paramName) const
{
	if (m_paramBindings.find(paramName) != m_paramBindings.end())
	{
		return m_paramBindings.at(paramName);
	}
	return -1;
}

void ShaderProgram_OpenGL::Reset()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ShaderProgram_OpenGL::UpdateParameterValue(unsigned int binding, EDescriptorType type, const std::shared_ptr<RawResource> pRes)
{
	switch (type)
	{
	case EDescriptorType::UniformBuffer:
	{
		auto pBuffer = std::static_pointer_cast<UniformBuffer_OpenGL>(pRes);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, pBuffer->GetGLBufferID());
		break;
	}
	case EDescriptorType::CombinedImageSampler:
	{
		std::shared_ptr<Texture2D_OpenGL> pTexture = nullptr;
		switch (std::static_pointer_cast<Texture2D>(pRes)->QuerySource())
		{
		case ETexture2DSource::ImageTexture:
			pTexture = std::static_pointer_cast<Texture2D_OpenGL>(std::static_pointer_cast<ImageTexture>(pRes)->GetTexture());
			break;

		case ETexture2DSource::RenderTexture:
			pTexture = std::static_pointer_cast<Texture2D_OpenGL>(std::static_pointer_cast<RenderTexture>(pRes)->GetTexture());
			break;

		case ETexture2DSource::RawDeviceTexture:
			pTexture = std::static_pointer_cast<Texture2D_OpenGL>(pRes);
			break;

		default:
			throw std::runtime_error("OpenGL: Unhandled texture 2D source type.");
			return;
		}

		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, pTexture->GetGLTextureID());
		break;
	}
	default:
		throw std::runtime_error("OpenGL: Unhandled shader parameter type.");
		break;
	}
}

void ShaderProgram_OpenGL::ReflectParamLocations()
{
	// Binding info cannot be retrieved at runtime, it will be manually assigned for compatibility with Vulkan

	// Combined image samplers

	m_paramBindings.emplace(ShaderParamNames::SHADOWMAP_DEPTH_TEXTURE, 0);

	m_paramBindings.emplace(ShaderParamNames::ALBEDO_TEXTURE, 1);

	m_paramBindings.emplace(ShaderParamNames::GCOLOR_TEXTURE, 20);
	m_paramBindings.emplace(ShaderParamNames::GNORMAL_TEXTURE, 2);
	m_paramBindings.emplace(ShaderParamNames::GPOSITION_TEXTURE, 3);

	m_paramBindings.emplace(ShaderParamNames::DEPTH_TEXTURE_1, 4);
	m_paramBindings.emplace(ShaderParamNames::DEPTH_TEXTURE_2, 5);

	m_paramBindings.emplace(ShaderParamNames::COLOR_TEXTURE_1, 6);
	m_paramBindings.emplace(ShaderParamNames::COLOR_TEXTURE_2, 7);

	m_paramBindings.emplace(ShaderParamNames::TONE_TEXTURE, 8);

	m_paramBindings.emplace(ShaderParamNames::NOISE_TEXTURE_1, 9);
	m_paramBindings.emplace(ShaderParamNames::NOISE_TEXTURE_2, 10);

	m_paramBindings.emplace(ShaderParamNames::MASK_TEXTURE_1, 11);
	m_paramBindings.emplace(ShaderParamNames::MASK_TEXTURE_2, 12);

	// Uniform blocks

	m_paramBindings.emplace(ShaderParamNames::TRANSFORM_MATRICES, 14);
	m_paramBindings.emplace(ShaderParamNames::LIGHTSPACE_TRANSFORM_MATRIX, 15);

	m_paramBindings.emplace(ShaderParamNames::MATERIAL_NUMERICAL_PROPERTIES, 16);

	m_paramBindings.emplace(ShaderParamNames::CAMERA_PROPERTIES, 17);

	m_paramBindings.emplace(ShaderParamNames::SYSTEM_VARIABLES, 18);
	m_paramBindings.emplace(ShaderParamNames::CONTROL_VARIABLES, 19);

	m_paramBindings.emplace(ShaderParamNames::LIGHTSOURCE_PROPERTIES, 21);

	// Next to be: 22
}

GraphicsPipeline_OpenGL::GraphicsPipeline_OpenGL(DrawingDevice_OpenGL* pDevice, const std::shared_ptr<ShaderProgram_OpenGL> pShaderProgram, GraphicsPipelineCreateInfo_OpenGL& createInfo)
	: m_pDevice(pDevice), m_pShaderProgram(pShaderProgram)
{
	m_primitiveTopologyMode = OpenGLAssemblyTopologyMode(createInfo.topologyMode);
	m_enablePrimitiveRestart = createInfo.enablePrimitiveRestart;

	m_enableBlend = createInfo.enableBlend;
	if (m_enableBlend)
	{
		m_blendSrcFactor = OpenGLBlendFactor(createInfo.blendSrcFactor);
		m_blendDstFactor = OpenGLBlendFactor(createInfo.blendDstFactor);
	}

	m_enableCulling = createInfo.enableCulling;
	if (m_enableCulling)
	{
		m_cullMode = OpenGLCullMode(createInfo.cullMode);
	}
	m_polygonMode = OpenGLPolygonMode(createInfo.polygonMode);

	m_enableDepthTest = createInfo.enableDepthTest;
	m_enableDepthMask = createInfo.enableDepthMask;

	m_enableMultisampling = createInfo.enableMultisampling;

	m_viewportWidth = createInfo.viewportWidth;
	m_viewportHeight = createInfo.viewportHeight;
}

void GraphicsPipeline_OpenGL::Apply() const
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

	glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

PipelineInputAssemblyState_OpenGL::PipelineInputAssemblyState_OpenGL(const PipelineInputAssemblyStateCreateInfo& createInfo)
{
	topologyMode = createInfo.topology;
	enablePrimitiveRestart = createInfo.enablePrimitiveRestart;
}

PipelineColorBlendState_OpenGL::PipelineColorBlendState_OpenGL(const PipelineColorBlendStateCreateInfo& createInfo)
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

PipelineRasterizationState_OpenGL::PipelineRasterizationState_OpenGL(const PipelineRasterizationStateCreateInfo& createInfo)
{
	enableCull = createInfo.cullMode == ECullMode::None ? false : true;
	cullMode = createInfo.cullMode;
	polygonMode = createInfo.polygonMode;
}

PipelineDepthStencilState_OpenGL::PipelineDepthStencilState_OpenGL(const PipelineDepthStencilStateCreateInfo& createInfo)
{
	enableDepthTest = createInfo.enableDepthTest;
	enableDepthMask = createInfo.enableDepthWrite;
}

PipelineMultisampleState_OpenGL::PipelineMultisampleState_OpenGL(const PipelineMultisampleStateCreateInfo& createInfo)
{
	enableMultisampling = createInfo.enableSampleShading;
}

PipelineViewportState_OpenGL::PipelineViewportState_OpenGL(const PipelineViewportStateCreateInfo& createInfo)
{
	viewportWidth = createInfo.width;
	viewportHeight = createInfo.height;
}