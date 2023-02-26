#include "GraphicsHardwareInterface_GL.h"
#include "GHIUtilities_GL.h"
#include "Buffers_GL.h"
#include "Textures_GL.h"
#include "Shaders_GL.h"
#include "Pipelines_GL.h"

namespace Engine
{
	GraphicsHardwareInterface_GL::~GraphicsHardwareInterface_GL()
	{
		ShutDown();
	}

	void GraphicsHardwareInterface_GL::Initialize()
	{
		glGenVertexArrays(1, &m_attributeless_vao);
	}

	void GraphicsHardwareInterface_GL::ShutDown()
	{

	}

	std::shared_ptr<ShaderProgram> GraphicsHardwareInterface_GL::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
	{
		auto pVertexShader = std::make_shared<VertexShader_GL>(vertexShaderFilePath);
		auto pFragmentShader = std::make_shared<FragmentShader_GL>(fragmentShaderFilePath);

		return std::make_shared<ShaderProgram_GL>(this, pVertexShader, pFragmentShader);
	}

	bool GraphicsHardwareInterface_GL::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput)
	{
		std::shared_ptr<VertexBuffer_GL> pVertexBuffer = std::make_shared<VertexBuffer_GL>();

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

	bool GraphicsHardwareInterface_GL::CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput)
	{
		if (pOutput != nullptr)
		{
			auto pTexture = std::static_pointer_cast<Texture2D_GL>(pOutput);
			GLuint texID = pTexture->GetGLTextureID();
			if (texID != -1)
			{
				glDeleteTextures(1, &texID);
			}
		}
		else
		{
			pOutput = std::make_shared<Texture2D_GL>();
		}

		auto pTexture = std::static_pointer_cast<Texture2D_GL>(pOutput);

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

	bool GraphicsHardwareInterface_GL::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput)
	{
		if (pOutput != nullptr)
		{
			auto pFrameBuffer = std::static_pointer_cast<FrameBuffer_GL>(pOutput);
			GLuint frameBufferID = pFrameBuffer->GetGLFrameBufferID();
			if (frameBufferID != -1)
			{
				glDeleteFramebuffers(1, &frameBufferID);
			}
		}
		else
		{
			pOutput = std::make_shared<FrameBuffer_GL>();
		}

		auto pFrameBuffer = std::static_pointer_cast<FrameBuffer_GL>(pOutput);

		GLuint frameBufferID = -1;
		glGenFramebuffers(1, &frameBufferID);

		glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);

		int colorAttachmentCount = 0;
		for (int i = 0; i < createInfo.attachments.size(); ++i)
		{
			switch (createInfo.attachments[i]->GetTextureType())
			{
			case ETextureType::ColorAttachment:
				pFrameBuffer->AddColorAttachment(GL_COLOR_ATTACHMENT0 + colorAttachmentCount); // Alert: this could cause the framebuffer to be partially "initialized" when creation failed
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentCount, GL_TEXTURE_2D, std::static_pointer_cast<Texture2D_GL>(createInfo.attachments[i])->GetGLTextureID(), 0);
				colorAttachmentCount++;
				break;
			case ETextureType::DepthAttachment:
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, std::static_pointer_cast<Texture2D_GL>(createInfo.attachments[i])->GetGLTextureID(), 0);
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

	bool GraphicsHardwareInterface_GL::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput)
	{
		pOutput = std::make_shared<UniformBuffer_GL>();

		auto pBuffer = std::static_pointer_cast<UniformBuffer_GL>(pOutput);

		GLuint bufferID = -1;
		glGenBuffers(1, &bufferID);
		pBuffer->SetGLBufferID(bufferID);
		pBuffer->MarkSizeInByte(createInfo.sizeInBytes);

		return bufferID != -1;
	}

	void GraphicsHardwareInterface_GL::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
	{
		SetRenderTarget(pFrameBuffer, std::vector<uint32_t>());
	}

	void GraphicsHardwareInterface_GL::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments)
	{
		if (pFrameBuffer == nullptr)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		auto pTarget = std::static_pointer_cast<FrameBuffer_GL>(pFrameBuffer);

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
				for (unsigned int i = 0; i < attachments.size(); ++i)
				{
					colorAttachments.emplace_back(pTarget->GetColorAttachment(attachments[i]));
				}
				glDrawBuffers((GLsizei)colorAttachments.size(), colorAttachments.data());
			}
		}
	}

	void GraphicsHardwareInterface_GL::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		auto pProgram = std::static_pointer_cast<ShaderProgram_GL>(pShaderProgram);

		for (auto& entry : pTable->m_table)
		{
			pProgram->UpdateParameterValue(entry.binding, entry.type, entry.pResource);
		}
	}

	void GraphicsHardwareInterface_GL::SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		if (!pVertexBuffer)
		{
			return;
		}

		glBindVertexArray(std::static_pointer_cast<VertexBuffer_GL>(pVertexBuffer)->m_vao);
	}

	void GraphicsHardwareInterface_GL::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		glDrawElementsBaseVertex(m_primitiveTopologyMode, indicesCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
	}

	void GraphicsHardwareInterface_GL::DrawFullScreenQuad(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		// This has to be used with FullScreenQuad shader
		glBindVertexArray(m_attributeless_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void GraphicsHardwareInterface_GL::ResizeViewPort(uint32_t width, uint32_t height)
	{
		glViewport(0, 0, width, height);
	}

	EGraphicsAPIType GraphicsHardwareInterface_GL::GetGraphicsAPIType() const
	{
		return EGraphicsAPIType::OpenGL;
	}

	std::shared_ptr<GraphicsCommandPool> GraphicsHardwareInterface_GL::RequestExternalCommandPool(EQueueType queueType)
	{
		LOG_ERROR("OpenGL: shouldn't call RequestExternalCommandPool on OpenGL device.");
		return nullptr;
	}

	std::shared_ptr<GraphicsCommandBuffer> GraphicsHardwareInterface_GL::RequestCommandBuffer(std::shared_ptr<GraphicsCommandPool> pCommandPool)
	{
		return nullptr;
	}

	void GraphicsHardwareInterface_GL::ReturnExternalCommandBuffer(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call ReturnExternalCommandBuffer on OpenGL device.");
	}

	std::shared_ptr<GraphicsSemaphore> GraphicsHardwareInterface_GL::RequestGraphicsSemaphore(ESemaphoreWaitStage waitStage)
	{
		LOG_ERROR("OpenGL: shouldn't call RequestGraphicsSemaphore on OpenGL device.");
		return nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, std::shared_ptr<DataTransferBuffer>& pOutput)
	{
		LOG_ERROR("OpenGL: shouldn't call CreateDataTransferBuffer on OpenGL device.");
		return false;
	}

	bool GraphicsHardwareInterface_GL::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput)
	{
		pOutput = std::make_shared<RenderPass_GL>();
		auto pRenderPass = std::static_pointer_cast<RenderPass_GL>(pOutput);

		for (int i = 0; i < createInfo.attachmentDescriptions.size(); i++)
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

	bool GraphicsHardwareInterface_GL::CreateSampler(const TextureSamplerCreateInfo& createInfo, std::shared_ptr<TextureSampler>& pOutput)
	{
		LOG_ERROR("OpenGL: shouldn't call CreateSampler on OpenGL device.");
		return false;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, std::shared_ptr<PipelineVertexInputState>& pOutput)
	{
		pOutput = nullptr;
		return false;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, std::shared_ptr<PipelineInputAssemblyState>& pOutput)
	{
		pOutput = std::make_shared<PipelineInputAssemblyState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, std::shared_ptr<PipelineColorBlendState>& pOutput)
	{
		pOutput = std::make_shared<PipelineColorBlendState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, std::shared_ptr<PipelineRasterizationState>& pOutput)
	{
		pOutput = std::make_shared<PipelineRasterizationState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, std::shared_ptr<PipelineDepthStencilState>& pOutput)
	{
		pOutput = std::make_shared<PipelineDepthStencilState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, std::shared_ptr<PipelineMultisampleState>& pOutput)
	{
		pOutput = std::make_shared<PipelineMultisampleState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, std::shared_ptr<PipelineViewportState>& pOutput)
	{
		pOutput = std::make_shared<PipelineViewportState_GL>(createInfo);
		return pOutput != nullptr;
	}

	bool GraphicsHardwareInterface_GL::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput)
	{
		GraphicsPipelineCreateInfo_GL glCreateInfo = {};

		glCreateInfo.topologyMode = std::static_pointer_cast<PipelineInputAssemblyState_GL>(createInfo.pInputAssemblyState)->topologyMode;
		glCreateInfo.enablePrimitiveRestart = std::static_pointer_cast<PipelineInputAssemblyState_GL>(createInfo.pInputAssemblyState)->enablePrimitiveRestart;

		glCreateInfo.enableBlend = std::static_pointer_cast<PipelineColorBlendState_GL>(createInfo.pColorBlendState)->enableBlend;
		glCreateInfo.blendSrcFactor = std::static_pointer_cast<PipelineColorBlendState_GL>(createInfo.pColorBlendState)->blendSrcFactor;
		glCreateInfo.blendDstFactor = std::static_pointer_cast<PipelineColorBlendState_GL>(createInfo.pColorBlendState)->blendDstFactor;

		glCreateInfo.enableCulling = std::static_pointer_cast<PipelineRasterizationState_GL>(createInfo.pRasterizationState)->enableCull;
		glCreateInfo.cullMode = std::static_pointer_cast<PipelineRasterizationState_GL>(createInfo.pRasterizationState)->cullMode;

		glCreateInfo.enableDepthTest = std::static_pointer_cast<PipelineDepthStencilState_GL>(createInfo.pDepthStencilState)->enableDepthTest;
		glCreateInfo.enableDepthMask = std::static_pointer_cast<PipelineDepthStencilState_GL>(createInfo.pDepthStencilState)->enableDepthMask;

		glCreateInfo.enableMultisampling = std::static_pointer_cast<PipelineMultisampleState_GL>(createInfo.pMultisampleState)->enableMultisampling;

		glCreateInfo.viewportWidth = std::static_pointer_cast<PipelineViewportState_GL>(createInfo.pViewportState)->viewportWidth;
		glCreateInfo.viewportHeight = std::static_pointer_cast<PipelineViewportState_GL>(createInfo.pViewportState)->viewportHeight;

		pOutput = std::make_shared<GraphicsPipeline_GL>(this, std::static_pointer_cast<ShaderProgram_GL>(createInfo.pShaderProgram), glCreateInfo);

		return pOutput != nullptr;
	}

	void GraphicsHardwareInterface_GL::TransitionImageLayout(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages)
	{
		LOG_ERROR("OpenGL: shouldn't call TransitionImageLayout on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::TransitionImageLayout_Immediate(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages)
	{
		LOG_ERROR("OpenGL: shouldn't call TransitionImageLayout_Immediate on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::ResizeSwapchain(uint32_t width, uint32_t height)
	{
		LOG_ERROR("OpenGL: shouldn't call ResizeSwapchain on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::BindGraphicsPipeline(const std::shared_ptr<GraphicsPipelineObject> pPipeline, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		std::static_pointer_cast<GraphicsPipeline_GL>(pPipeline)->Apply();
	}

	void GraphicsHardwareInterface_GL::BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		SetRenderTarget(pFrameBuffer);
		std::static_pointer_cast<RenderPass_GL>(pRenderPass)->Initialize();
	}

	void GraphicsHardwareInterface_GL::EndRenderPass(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call EndRenderPass on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::EndCommandBuffer(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call EndCommandBuffer on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CommandWaitSemaphore(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer, std::shared_ptr<GraphicsSemaphore> pSemaphore)
	{
		LOG_ERROR("OpenGL: shouldn't call CommandWaitSemaphore on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CommandSignalSemaphore(std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer, std::shared_ptr<GraphicsSemaphore> pSemaphore)
	{
		LOG_ERROR("OpenGL: shouldn't call CommandSignalSemaphore on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::Present()
	{
		LOG_ERROR("OpenGL: shouldn't call Present on OpenGL device.");
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
		LOG_ERROR("OpenGL: shouldn't call FlushTransferCommands on OpenGL device.");
		glFlush();
	}

	void GraphicsHardwareInterface_GL::WaitSemaphore(std::shared_ptr<GraphicsSemaphore> pSemaphore)
	{
		LOG_ERROR("OpenGL: shouldn't call WaitSemaphore on OpenGL device.");
		glFinish();
	}

	std::shared_ptr<TextureSampler> GraphicsHardwareInterface_GL::GetDefaultTextureSampler(bool withDefaultAF) const
	{
		return nullptr;
	}

	void GraphicsHardwareInterface_GL::GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const
	{
		LOG_ERROR("OpenGL: shouldn't call GetSwapchainImages on OpenGL device.");
	}

	uint32_t GraphicsHardwareInterface_GL::GetSwapchainPresentImageIndex() const
	{
		LOG_ERROR("OpenGL: shouldn't call GetSwapchainPresentImageIndex on OpenGL device.");
		return -1;
	}

	void GraphicsHardwareInterface_GL::CopyTexture2DToDataTransferBuffer(std::shared_ptr<Texture2D> pSrcTexture, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call CopyTexture2DToDataTransferBuffer on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CopyDataTransferBufferToTexture2D(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<Texture2D> pDstTexture, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call CopyDataTransferBufferToTexture2D on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CopyDataTransferBuffer(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<GraphicsCommandBuffer> pCommandBuffer)
	{
		LOG_ERROR("OpenGL: shouldn't call CopyDataTransferBuffer on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CopyHostDataToDataTransferBuffer(void* pData, std::shared_ptr<DataTransferBuffer> pDstBuffer, size_t size)
	{
		LOG_ERROR("OpenGL: shouldn't call CopyHostDataToDataTransferBuffer on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::CopyDataTransferBufferToHostDataLocation(std::shared_ptr<DataTransferBuffer> pSrcBuffer, void* pDataLoc)
	{
		LOG_ERROR("OpenGL: shouldn't call CopyDataTransferBufferToHostDataLocation on OpenGL device.");
	}

	void GraphicsHardwareInterface_GL::SetPrimitiveTopology(GLenum mode)
	{
		m_primitiveTopologyMode = mode;
	}
}