#include "GraphicsHardwareInterface_GL.h"
#include "GHIUtilities_GL.h"
#include "Buffers_GL.h"
#include "Textures_GL.h"
#include "Shaders_GL.h"
#include "Pipelines_GL.h"
#include "MemoryAllocator.h"

// For now OpenGL always prefers discrete GPU regardless of graphics configuration
// Full implementation requires WGL_NV_gpu_affinity and WGL_AMD_gpu_association extensions
#if defined(PLATFORM_WINDOWS_CE)
typedef unsigned long DWORD;
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace Engine
{
	GraphicsHardwareInterface_GL::GraphicsHardwareInterface_GL()
		: m_attributeless_vao(-1),
		m_primitiveTopologyMode(GL_TRIANGLES)
	{

	}

	GraphicsHardwareInterface_GL::~GraphicsHardwareInterface_GL()
	{
		ShutDown();
	}

	void GraphicsHardwareInterface_GL::Initialize()
	{
		GraphicsDevice::Initialize();

		glGenVertexArrays(1, &m_attributeless_vao);

		// Temporary solution for enabling sRGB support
		glEnable(GL_FRAMEBUFFER_SRGB);
	}

	void GraphicsHardwareInterface_GL::ShutDown()
	{
		GraphicsDevice::ShutDown();
	}

	ShaderProgram* GraphicsHardwareInterface_GL::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
	{
		VertexShader_GL* pVertexShader;
		CE_NEW(pVertexShader , VertexShader_GL, vertexShaderFilePath);
		FragmentShader_GL* pFragmentShader;
		CE_NEW(pFragmentShader, FragmentShader_GL, fragmentShaderFilePath);

		ShaderProgram_GL* pResult;
		CE_NEW(pResult, ShaderProgram_GL, this, pVertexShader, pFragmentShader);
		return pResult;
	}

	bool GraphicsHardwareInterface_GL::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, VertexBuffer*& pOutput)
	{
		VertexBuffer_GL* pVertexBuffer;
		CE_NEW(pVertexBuffer, VertexBuffer_GL);

		glGenVertexArrays(1, &pVertexBuffer->m_vao);
		glBindVertexArray(pVertexBuffer->m_vao);

		// Index
		glGenBuffers(1, &pVertexBuffer->m_vboIndices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pVertexBuffer->m_vboIndices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * createInfo.indexDataCount, createInfo.pIndexData, GL_STATIC_DRAW);

		std::vector<float> interleavedVertices = createInfo.ConvertToInterleavedData();

		glGenBuffers(1, &pVertexBuffer->m_vboVertices);
		glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboVertices);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * interleavedVertices.size(), interleavedVertices.data(), GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(ATTRIB_POSITION_LOCATION);
		glVertexAttribPointer(ATTRIB_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, createInfo.interleavedStride, (void*)createInfo.positionOffset);

		// Normal
		glEnableVertexAttribArray(ATTRIB_NORMAL_LOCATION);
		glVertexAttribPointer(ATTRIB_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, createInfo.interleavedStride, (void*)createInfo.normalOffset);

		// TexCoord
		glEnableVertexAttribArray(ATTRIB_TEXCOORD_LOCATION);
		glVertexAttribPointer(ATTRIB_TEXCOORD_LOCATION, 2, GL_FLOAT, GL_FALSE, createInfo.interleavedStride, (void*)createInfo.texcoordOffset);

		// Tangent
		glEnableVertexAttribArray(ATTRIB_TANGENT_LOCATION);
		glVertexAttribPointer(ATTRIB_TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, createInfo.interleavedStride, (void*)createInfo.tangentOffset);

		// Bitangent
		glEnableVertexAttribArray(ATTRIB_BITANGENT_LOCATION);
		glVertexAttribPointer(ATTRIB_BITANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, createInfo.interleavedStride, (void*)createInfo.bitangentOffset);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		pVertexBuffer->SetNumberOfIndices(createInfo.indexDataCount);
		pVertexBuffer->MarkSizeInByte(static_cast<uint32_t>(sizeof(int) * createInfo.indexDataCount
			+ sizeof(float) * createInfo.positionDataCount
			+ sizeof(float) * createInfo.normalDataCount
			+ sizeof(float) * createInfo.texcoordDataCount
			+ sizeof(float) * createInfo.tangentDataCount
			+ sizeof(float) * createInfo.bitangentDataCount
			));

		pOutput = pVertexBuffer;
		return true;
	}

	bool GraphicsHardwareInterface_GL::CreateTexture2D(const Texture2DCreateInfo& createInfo, Texture2D*& pOutput)
	{
		if (pOutput != nullptr)
		{
			auto pTexture = (Texture2D_GL*)pOutput;
			GLuint texID = pTexture->GetGLTextureID();
			if (texID != -1)
			{
				glDeleteTextures(1, &texID);
			}
		}
		else
		{
			CE_NEW(pOutput, Texture2D_GL);
		}

		auto pTexture = (Texture2D_GL*)pOutput;

		GLuint texID = -1;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexImage2D(GL_TEXTURE_2D, 0, OpenGLFormat(createInfo.format), createInfo.textureWidth, createInfo.textureHeight, 0, OpenGLPixelFormat(OpenGLFormat(createInfo.format)), OpenGLDataType(createInfo.dataType), createInfo.pTextureData);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (createInfo.format == ETextureFormat::Depth)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		}
		if (createInfo.generateMipmap)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		pTexture->SetGLTextureID(texID);
		pTexture->MarkTextureSize(createInfo.textureWidth, createInfo.textureHeight);
		pTexture->SetTextureType(createInfo.textureType);
		pTexture->MarkSizeInByte(static_cast<uint32_t>(createInfo.textureWidth * createInfo.textureHeight * OpenGLTypeSize(createInfo.dataType)));

		return texID != -1;
	}

	bool GraphicsHardwareInterface_GL::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, FrameBuffer*& pOutput)
	{
		if (pOutput != nullptr)
		{
			auto pFrameBuffer = (FrameBuffer_GL*)pOutput;
			GLuint frameBufferID = pFrameBuffer->GetGLFrameBufferID();
			if (frameBufferID != -1)
			{
				glDeleteFramebuffers(1, &frameBufferID);
			}
		}
		else
		{
			CE_NEW(pOutput, FrameBuffer_GL);
		}

		auto pFrameBuffer = (FrameBuffer_GL*)pOutput;

		if (createInfo.renderToSwapchain)
		{
			pFrameBuffer->MarkRenderToBackbuffer();
			return true;
		}

		GLuint frameBufferID = -1;
		glGenFramebuffers(1, &frameBufferID);

		glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);

		uint32_t colorAttachmentCount = 0;
		for (uint32_t i = 0; i < createInfo.attachments.size(); ++i)
		{
			switch (createInfo.attachments[i]->GetTextureType())
			{
			case ETextureType::ColorAttachment:
				pFrameBuffer->AddColorAttachment(GL_COLOR_ATTACHMENT0 + colorAttachmentCount); // Alert: this could cause the framebuffer to be partially "initialized" when creation failed
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentCount, GL_TEXTURE_2D, ((Texture2D_GL*)createInfo.attachments[i])->GetGLTextureID(), 0);
				colorAttachmentCount++;
				break;
			case ETextureType::DepthAttachment:
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ((Texture2D_GL*)createInfo.attachments[i])->GetGLTextureID(), 0);
				break;
			default:
				LOG_ERROR("OpenGL: Unhandled framebuffer attachment type.");
				break;
			}
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			LOG_ERROR("OpenGL: Failed to create framebuffer.");
			return false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		pFrameBuffer->SetGLFrameBufferID(frameBufferID);
		pFrameBuffer->MarkFrameBufferSize(createInfo.framebufferWidth, createInfo.framebufferHeight);

		return frameBufferID != -1;
	}

	bool GraphicsHardwareInterface_GL::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, UniformBuffer*& pOutput)
	{
		CE_NEW(pOutput, UniformBuffer_GL);

		auto pBuffer = (UniformBuffer_GL*)pOutput;

		GLuint bufferID = -1;
		glGenBuffers(1, &bufferID);
		pBuffer->SetGLBufferID(bufferID);
		pBuffer->MarkSizeInByte(createInfo.sizeInBytes);

		return bufferID != -1;
	}

	void GraphicsHardwareInterface_GL::SetRenderTarget(const FrameBuffer* pFrameBuffer)
	{
		SetRenderTarget(pFrameBuffer, std::vector<uint32_t>());
	}

	void GraphicsHardwareInterface_GL::SetRenderTarget(const FrameBuffer* pFrameBuffer, const std::vector<uint32_t>& attachments)
	{
		if (pFrameBuffer == nullptr)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		auto pTarget = (FrameBuffer_GL*)pFrameBuffer;

		glBindFramebuffer(GL_FRAMEBUFFER, pTarget->GetGLFrameBufferID());

		if (attachments.size() == 0) // Default to draw all attachments
		{
			glDrawBuffers((GLsizei)pTarget->GetColorAttachmentCount(), pTarget->GetColorAttachments());
		}
		else
		{
			if (attachments.size() == 1)
			{
				glDrawBuffer(pTarget->GetColorAttachment(attachments[0]));
			}
			else
			{
				std::vector<GLenum> colorAttachments;
				for (uint32_t i = 0; i < attachments.size(); ++i)
				{
					colorAttachments.emplace_back(pTarget->GetColorAttachment(attachments[i]));
				}
				glDrawBuffers((GLsizei)colorAttachments.size(), colorAttachments.data());
			}
		}
	}

	void GraphicsHardwareInterface_GL::UpdateShaderParameter(ShaderProgram* pShaderProgram, const ShaderParameterTable* pTable, GraphicsCommandBuffer* pCommandBuffer)
	{
		auto pProgram = (ShaderProgram_GL*)pShaderProgram;

		for (auto& entry : pTable->m_table)
		{
			pProgram->UpdateParameterValue(entry.binding, entry.type, entry.pResource);
		}
	}

	void GraphicsHardwareInterface_GL::SetVertexBuffer(const VertexBuffer* pVertexBuffer, GraphicsCommandBuffer* pCommandBuffer)
	{
		if (!pVertexBuffer)
		{
			return;
		}

		glBindVertexArray(((VertexBuffer_GL*)pVertexBuffer)->m_vao);
	}

	void GraphicsHardwareInterface_GL::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, GraphicsCommandBuffer* pCommandBuffer)
	{
		glDrawElementsBaseVertex(m_primitiveTopologyMode, indicesCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * baseIndex), baseVertex);
	}

	void GraphicsHardwareInterface_GL::DrawFullScreenQuad(GraphicsCommandBuffer* pCommandBuffer)
	{
		// This has to be used with FullScreenQuad shader
		glBindVertexArray(m_attributeless_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GraphicsHardwareInterface_GL::ResizeViewPort(uint32_t width, uint32_t height)
	{
		glViewport(0, 0, width, height);
	}

	EGraphicsAPIType GraphicsHardwareInterface_GL::GetGraphicsAPIType() const
	{
		return EGraphicsAPIType::OpenGL;
	}

	bool GraphicsHardwareInterface_GL::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, RenderPassObject*& pOutput)
	{
		CE_NEW(pOutput, RenderPass_GL);
		auto pRenderPass = (RenderPass_GL*)pOutput;

		for (uint32_t i = 0; i < createInfo.attachmentDescriptions.size(); i++)
		{
			if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Color)
			{
				pRenderPass->m_clearColorOnLoad = pRenderPass->m_clearColorOnLoad || (createInfo.attachmentDescriptions[i].loadOp == EAttachmentOperation::Clear);
			}
			else if (createInfo.attachmentDescriptions[i].type == EAttachmentType::Depth)
			{
				pRenderPass->m_clearDepthOnLoad = (createInfo.attachmentDescriptions[i].loadOp == EAttachmentOperation::Clear);
			}
		}

		if (pRenderPass->m_clearColorOnLoad)
		{
			pRenderPass->m_clearColor = createInfo.clearColor;
		}

		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreateSampler(const TextureSamplerCreateInfo& createInfo, TextureSampler*& pOutput)
	{
		CE_NEW(pOutput, Sampler_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, PipelineInputAssemblyState*& pOutput)
	{
		CE_NEW(pOutput, PipelineInputAssemblyState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, PipelineColorBlendState*& pOutput)
	{
		CE_NEW(pOutput, PipelineColorBlendState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, PipelineRasterizationState*& pOutput)
	{
		CE_NEW(pOutput, PipelineRasterizationState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, PipelineDepthStencilState*& pOutput)
	{
		CE_NEW(pOutput, PipelineDepthStencilState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, PipelineMultisampleState*& pOutput)
	{
		CE_NEW(pOutput, PipelineMultisampleState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, PipelineViewportState*& pOutput)
	{
		CE_NEW(pOutput, PipelineViewportState_GL, createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipelineObject*& pOutput)
	{
		GraphicsPipelineCreateInfo_GL glCreateInfo{};

		glCreateInfo.topologyMode = ((PipelineInputAssemblyState_GL*)createInfo.pInputAssemblyState)->topologyMode;
		glCreateInfo.enablePrimitiveRestart = ((PipelineInputAssemblyState_GL*)createInfo.pInputAssemblyState)->enablePrimitiveRestart;

		glCreateInfo.enableBlend = ((PipelineColorBlendState_GL*)createInfo.pColorBlendState)->enableBlend;
		glCreateInfo.blendSrcFactor = ((PipelineColorBlendState_GL*)createInfo.pColorBlendState)->blendSrcFactor;
		glCreateInfo.blendDstFactor = ((PipelineColorBlendState_GL*)createInfo.pColorBlendState)->blendDstFactor;

		glCreateInfo.enableCulling = ((PipelineRasterizationState_GL*)createInfo.pRasterizationState)->enableCull;
		glCreateInfo.cullMode = ((PipelineRasterizationState_GL*)createInfo.pRasterizationState)->cullMode;

		glCreateInfo.enableDepthTest = ((PipelineDepthStencilState_GL*)createInfo.pDepthStencilState)->enableDepthTest;
		glCreateInfo.enableDepthMask = ((PipelineDepthStencilState_GL*)createInfo.pDepthStencilState)->enableDepthMask;

		glCreateInfo.enableMultisampling = ((PipelineMultisampleState_GL*)createInfo.pMultisampleState)->enableMultisampling;

		glCreateInfo.viewportWidth = ((PipelineViewportState_GL*)createInfo.pViewportState)->viewportWidth;
		glCreateInfo.viewportHeight = ((PipelineViewportState_GL*)createInfo.pViewportState)->viewportHeight;

		CE_NEW(pOutput, GraphicsPipeline_GL, this, (ShaderProgram_GL*)createInfo.pShaderProgram, glCreateInfo);

		return pOutput != nullptr;
	}

	void GraphicsHardwareInterface_GL::BindGraphicsPipeline(const GraphicsPipelineObject* pPipeline, GraphicsCommandBuffer* pCommandBuffer)
	{
		((GraphicsPipeline_GL*)pPipeline)->Apply();
	}

	void GraphicsHardwareInterface_GL::BeginRenderPass(const RenderPassObject* pRenderPass, const FrameBuffer* pFrameBuffer, GraphicsCommandBuffer* pCommandBuffer)
	{
		if (!((FrameBuffer_GL*)pFrameBuffer)->IsRenderToBackbuffer())
		{
			SetRenderTarget(pFrameBuffer);
		}
		else
		{
			SetRenderTarget(nullptr);
		}
		((RenderPass_GL*)pRenderPass)->Initialize();
	}

	void GraphicsHardwareInterface_GL::EndRenderPass(GraphicsCommandBuffer* pCommandBuffer)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GraphicsHardwareInterface_GL::FlushCommands(bool waitExecution, bool flushImplicitCommands)
	{
		glFlush();

		if (waitExecution)
		{
			glFinish();
		}
	}

	void GraphicsHardwareInterface_GL::FlushTransferCommands(bool waitExecution)
	{
		LOG_WARNING("OpenGL: shouldn't call FlushTransferCommands on OpenGL device.");
		glFlush();
	}

	void GraphicsHardwareInterface_GL::WaitSemaphore(GraphicsSemaphore* pSemaphore)
	{
		LOG_WARNING("OpenGL: shouldn't call WaitSemaphore on OpenGL device.");
		glFinish();
	}

	void GraphicsHardwareInterface_GL::GetSwapchainImages(std::vector<Texture2D*>& outImages) const
	{
		uint32_t framesInFlight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight();
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			outImages.push_back(nullptr);
		}
	}

	void GraphicsHardwareInterface_GL::SetPrimitiveTopology(GLenum mode)
	{
		m_primitiveTopologyMode = mode;
	}
}