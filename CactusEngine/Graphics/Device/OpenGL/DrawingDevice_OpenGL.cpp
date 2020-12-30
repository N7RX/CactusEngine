#include "DrawingDevice_OpenGL.h"
#include "DrawingResources_OpenGL.h"
#include "DrawingUtil_OpenGL.h"

using namespace Engine;

DrawingDevice_OpenGL::~DrawingDevice_OpenGL()
{
	ShutDown();
}

void DrawingDevice_OpenGL::Initialize()
{
	glGenVertexArrays(1, &m_attributeless_vao);
}

void DrawingDevice_OpenGL::ShutDown()
{

}

std::shared_ptr<ShaderProgram> DrawingDevice_OpenGL::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
{
	auto pVertexShader = std::make_shared<VertexShader_OpenGL>(vertexShaderFilePath);
	auto pFragmentShader = std::make_shared<FragmentShader_OpenGL>(fragmentShaderFilePath);

	return std::make_shared<ShaderProgram_OpenGL>(this, pVertexShader, pFragmentShader);
}

std::shared_ptr<ShaderProgram> DrawingDevice_OpenGL::CreateShaderProgramFromFile(const char* vertexShaderFilePath, const char* fragmentShaderFilePath, EGPUType gpuType)
{
	// GPU type specification will be ignored in OpenGL implementation
	return CreateShaderProgramFromFile(vertexShaderFilePath, fragmentShaderFilePath);
}

bool DrawingDevice_OpenGL::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput)
{
	std::shared_ptr<VertexBuffer_OpenGL> pVertexBuffer = std::make_shared<VertexBuffer_OpenGL>();
	
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

bool DrawingDevice_OpenGL::CreateTexture2D(const Texture2DCreateInfo& createInfo, std::shared_ptr<Texture2D>& pOutput)
{
	if (pOutput != nullptr)
	{
		auto pTexture = std::static_pointer_cast<Texture2D_OpenGL>(pOutput);
		GLuint texID = pTexture->GetGLTextureID();
		if (texID != -1)
		{
			glDeleteTextures(1, &texID);
		}
	}
	else
	{
		pOutput = std::make_shared<Texture2D_OpenGL>();
	}

	auto pTexture = std::static_pointer_cast<Texture2D_OpenGL>(pOutput);

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

bool DrawingDevice_OpenGL::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo, std::shared_ptr<FrameBuffer>& pOutput)
{
	if (pOutput != nullptr)
	{
		auto pFrameBuffer = std::static_pointer_cast<FrameBuffer_OpenGL>(pOutput);
		GLuint frameBufferID = pFrameBuffer->GetGLFrameBufferID();
		if (frameBufferID != -1)
		{
			glDeleteFramebuffers(1, &frameBufferID);
		}
	}
	else
	{
		pOutput = std::make_shared<FrameBuffer_OpenGL>();
	}

	auto pFrameBuffer = std::static_pointer_cast<FrameBuffer_OpenGL>(pOutput);

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
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentCount, GL_TEXTURE_2D, std::static_pointer_cast<Texture2D_OpenGL>(createInfo.attachments[i])->GetGLTextureID(), 0);
			colorAttachmentCount++;
			break;
		case ETextureType::DepthAttachment:
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, std::static_pointer_cast<Texture2D_OpenGL>(createInfo.attachments[i])->GetGLTextureID(), 0);
			break;
		default:
			std::cerr << "OpenGL: Unhandled framebuffer attachment type.\n";
			break;
		}
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "OpenGL: Failed to create framebuffer.\n";
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	pFrameBuffer->SetGLFrameBufferID(frameBufferID);
	pFrameBuffer->MarkFrameBufferSize(createInfo.framebufferWidth, createInfo.framebufferHeight);

	return frameBufferID != -1;
}

bool DrawingDevice_OpenGL::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo, std::shared_ptr<UniformBuffer>& pOutput)
{
	pOutput = std::make_shared<UniformBuffer_OpenGL>();

	auto pBuffer = std::static_pointer_cast<UniformBuffer_OpenGL>(pOutput);

	GLuint bufferID = -1;
	glGenBuffers(1, &bufferID);
	pBuffer->SetGLBufferID(bufferID);
	pBuffer->MarkSizeInByte(createInfo.sizeInBytes);

	return bufferID != -1;
}

void DrawingDevice_OpenGL::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer)
{
	SetRenderTarget(pFrameBuffer, std::vector<uint32_t>());
}

void DrawingDevice_OpenGL::SetRenderTarget(const std::shared_ptr<FrameBuffer> pFrameBuffer, const std::vector<uint32_t>& attachments)
{
	if (pFrameBuffer == nullptr)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	auto pTarget = std::static_pointer_cast<FrameBuffer_OpenGL>(pFrameBuffer);

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

void DrawingDevice_OpenGL::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	auto pProgram = std::static_pointer_cast<ShaderProgram_OpenGL>(pShaderProgram);

	for (auto& entry : pTable->m_table)
	{
		pProgram->UpdateParameterValue(entry.binding, entry.type, entry.pResource);
	}
}

void DrawingDevice_OpenGL::SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	if (!pVertexBuffer)
	{
		return;
	}

	glBindVertexArray(std::static_pointer_cast<VertexBuffer_OpenGL>(pVertexBuffer)->m_vao);
}

void DrawingDevice_OpenGL::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	glDrawElementsBaseVertex(m_primitiveTopologyMode, indicesCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int)*baseIndex), baseVertex);
}

void DrawingDevice_OpenGL::DrawFullScreenQuad(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	// This has to be used with FullScreenQuad shader
	glBindVertexArray(m_attributeless_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void DrawingDevice_OpenGL::ResizeViewPort(uint32_t width, uint32_t height)
{
	glViewport(0, 0, width, height);
}

EGraphicsDeviceType DrawingDevice_OpenGL::GetDeviceType() const
{
	return EGraphicsDeviceType::OpenGL;
}

std::shared_ptr<DrawingCommandPool> DrawingDevice_OpenGL::RequestExternalCommandPool(EQueueType queueType, EGPUType deviceType)
{
	std::cerr << "OpenGL: shouldn't call RequestExternalCommandPool on OpenGL device.\n";
	return nullptr;
}

std::shared_ptr<DrawingCommandBuffer> DrawingDevice_OpenGL::RequestCommandBuffer(std::shared_ptr<DrawingCommandPool> pCommandPool)
{
	return nullptr;
}

void DrawingDevice_OpenGL::ReturnExternalCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call ReturnExternalCommandBuffer on OpenGL device.\n";
}

std::shared_ptr<DrawingSemaphore> DrawingDevice_OpenGL::RequestDrawingSemaphore(EGPUType deviceType, ESemaphoreWaitStage waitStage)
{
	std::cerr << "OpenGL: shouldn't call RequestDrawingSemaphore on OpenGL device.\n";
	return nullptr;
}

bool DrawingDevice_OpenGL::CreateDataTransferBuffer(const DataTransferBufferCreateInfo& createInfo, std::shared_ptr<DataTransferBuffer>& pOutput)
{
	std::cerr << "OpenGL: shouldn't call CreateDataTransferBuffer on OpenGL device.\n";
	return false;
}

bool DrawingDevice_OpenGL::CreateRenderPassObject(const RenderPassCreateInfo& createInfo, std::shared_ptr<RenderPassObject>& pOutput)
{
	pOutput = std::make_shared<RenderPass_OpenGL>();
	auto pRenderPass = std::static_pointer_cast<RenderPass_OpenGL>(pOutput);

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

bool DrawingDevice_OpenGL::CreateSampler(const TextureSamplerCreateInfo& createInfo, std::shared_ptr<TextureSampler>& pOutput)
{
	std::cerr << "OpenGL: shouldn't call CreateSampler on OpenGL device.\n";
	return false;
}

bool DrawingDevice_OpenGL::CreatePipelineVertexInputState(const PipelineVertexInputStateCreateInfo& createInfo, std::shared_ptr<PipelineVertexInputState>& pOutput)
{
	pOutput = nullptr;
	return false;
}

bool DrawingDevice_OpenGL::CreatePipelineInputAssemblyState(const PipelineInputAssemblyStateCreateInfo& createInfo, std::shared_ptr<PipelineInputAssemblyState>& pOutput)
{
	pOutput = std::make_shared<PipelineInputAssemblyState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreatePipelineColorBlendState(const PipelineColorBlendStateCreateInfo& createInfo, std::shared_ptr<PipelineColorBlendState>& pOutput)
{
	pOutput = std::make_shared<PipelineColorBlendState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreatePipelineRasterizationState(const PipelineRasterizationStateCreateInfo& createInfo, std::shared_ptr<PipelineRasterizationState>& pOutput)
{
	pOutput = std::make_shared<PipelineRasterizationState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreatePipelineDepthStencilState(const PipelineDepthStencilStateCreateInfo& createInfo, std::shared_ptr<PipelineDepthStencilState>& pOutput)
{
	pOutput = std::make_shared<PipelineDepthStencilState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreatePipelineMultisampleState(const PipelineMultisampleStateCreateInfo& createInfo, std::shared_ptr<PipelineMultisampleState>& pOutput)
{
	pOutput = std::make_shared<PipelineMultisampleState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreatePipelineViewportState(const PipelineViewportStateCreateInfo& createInfo, std::shared_ptr<PipelineViewportState>& pOutput)
{
	pOutput = std::make_shared<PipelineViewportState_OpenGL>(createInfo);
	return pOutput != nullptr;
}

bool DrawingDevice_OpenGL::CreateGraphicsPipelineObject(const GraphicsPipelineCreateInfo& createInfo, std::shared_ptr<GraphicsPipelineObject>& pOutput)
{
	GraphicsPipelineCreateInfo_OpenGL glCreateInfo = {};

	glCreateInfo.topologyMode = std::static_pointer_cast<PipelineInputAssemblyState_OpenGL>(createInfo.pInputAssemblyState)->topologyMode;
	glCreateInfo.enablePrimitiveRestart = std::static_pointer_cast<PipelineInputAssemblyState_OpenGL>(createInfo.pInputAssemblyState)->enablePrimitiveRestart;

	glCreateInfo.enableBlend = std::static_pointer_cast<PipelineColorBlendState_OpenGL>(createInfo.pColorBlendState)->enableBlend;
	glCreateInfo.blendSrcFactor = std::static_pointer_cast<PipelineColorBlendState_OpenGL>(createInfo.pColorBlendState)->blendSrcFactor;
	glCreateInfo.blendDstFactor = std::static_pointer_cast<PipelineColorBlendState_OpenGL>(createInfo.pColorBlendState)->blendDstFactor;

	glCreateInfo.enableCulling = std::static_pointer_cast<PipelineRasterizationState_OpenGL>(createInfo.pRasterizationState)->enableCull;
	glCreateInfo.cullMode = std::static_pointer_cast<PipelineRasterizationState_OpenGL>(createInfo.pRasterizationState)->cullMode;

	glCreateInfo.enableDepthTest = std::static_pointer_cast<PipelineDepthStencilState_OpenGL>(createInfo.pDepthStencilState)->enableDepthTest;
	glCreateInfo.enableDepthMask = std::static_pointer_cast<PipelineDepthStencilState_OpenGL>(createInfo.pDepthStencilState)->enableDepthMask;

	glCreateInfo.enableMultisampling = std::static_pointer_cast<PipelineMultisampleState_OpenGL>(createInfo.pMultisampleState)->enableMultisampling;

	glCreateInfo.viewportWidth = std::static_pointer_cast<PipelineViewportState_OpenGL>(createInfo.pViewportState)->viewportWidth;
	glCreateInfo.viewportHeight = std::static_pointer_cast<PipelineViewportState_OpenGL>(createInfo.pViewportState)->viewportHeight;

	pOutput = std::make_shared<GraphicsPipeline_OpenGL>(this, std::static_pointer_cast<ShaderProgram_OpenGL>(createInfo.pShaderProgram), glCreateInfo);

	return pOutput != nullptr;
}

void DrawingDevice_OpenGL::TransitionImageLayout(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages)
{
	std::cerr << "OpenGL: shouldn't call TransitionImageLayout on OpenGL device.\n";
}

void DrawingDevice_OpenGL::TransitionImageLayout_Immediate(std::shared_ptr<Texture2D> pImage, EImageLayout newLayout, uint32_t appliedStages)
{
	std::cerr << "OpenGL: shouldn't call TransitionImageLayout_Immediate on OpenGL device.\n";
}

void DrawingDevice_OpenGL::ResizeSwapchain(uint32_t width, uint32_t height)
{
	std::cerr << "OpenGL: shouldn't call ResizeSwapchain on OpenGL device.\n";
}

void DrawingDevice_OpenGL::BindGraphicsPipeline(const std::shared_ptr<GraphicsPipelineObject> pPipeline, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::static_pointer_cast<GraphicsPipeline_OpenGL>(pPipeline)->Apply();
}

void DrawingDevice_OpenGL::BeginRenderPass(const std::shared_ptr<RenderPassObject> pRenderPass, const std::shared_ptr<FrameBuffer> pFrameBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	SetRenderTarget(pFrameBuffer);
	std::static_pointer_cast<RenderPass_OpenGL>(pRenderPass)->Initialize();
}

void DrawingDevice_OpenGL::EndRenderPass(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call EndRenderPass on OpenGL device.\n";
}

void DrawingDevice_OpenGL::EndCommandBuffer(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call EndCommandBuffer on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CommandWaitSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore)
{
	std::cerr << "OpenGL: shouldn't call CommandWaitSemaphore on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CommandSignalSemaphore(std::shared_ptr<DrawingCommandBuffer> pCommandBuffer, std::shared_ptr<DrawingSemaphore> pSemaphore)
{
	std::cerr << "OpenGL: shouldn't call CommandSignalSemaphore on OpenGL device.\n";
}

void DrawingDevice_OpenGL::Present()
{
	std::cerr << "OpenGL: shouldn't call Present on OpenGL device.\n";
}

void DrawingDevice_OpenGL::FlushCommands(bool waitExecution, bool flushImplicitCommands, uint32_t deviceTypeFlags)
{
	glFlush();

	if (waitExecution)
	{
		glFinish();
	}
}

void DrawingDevice_OpenGL::FlushTransferCommands(bool waitExecution)
{
	std::cerr << "OpenGL: shouldn't call FlushTransferCommands on OpenGL device.\n";
	glFlush();
}

void DrawingDevice_OpenGL::WaitSemaphore(std::shared_ptr<DrawingSemaphore> pSemaphore)
{
	std::cerr << "OpenGL: shouldn't call WaitSemaphore on OpenGL device.\n";
	glFinish();
}

std::shared_ptr<TextureSampler> DrawingDevice_OpenGL::GetDefaultTextureSampler(EGPUType deviceType, bool withDefaultAF) const
{
	return nullptr;
}

void DrawingDevice_OpenGL::GetSwapchainImages(std::vector<std::shared_ptr<Texture2D>>& outImages) const
{
	std::cerr << "OpenGL: shouldn't call GetSwapchainImages on OpenGL device.\n";
}

uint32_t DrawingDevice_OpenGL::GetSwapchainPresentImageIndex() const
{
	std::cerr << "OpenGL: shouldn't call GetSwapchainPresentImageIndex on OpenGL device.\n";
	return -1;
}

void DrawingDevice_OpenGL::CopyTexture2DToDataTransferBuffer(std::shared_ptr<Texture2D> pSrcTexture, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call CopyTexture2DToDataTransferBuffer on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CopyDataTransferBufferToTexture2D(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<Texture2D> pDstTexture, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call CopyDataTransferBufferToTexture2D on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CopyDataTransferBufferCrossDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer)
{
	std::cerr << "OpenGL: shouldn't call CopyDataTransferBufferCrossDevice on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CopyDataTransferBufferWithinDevice(std::shared_ptr<DataTransferBuffer> pSrcBuffer, std::shared_ptr<DataTransferBuffer> pDstBuffer, std::shared_ptr<DrawingCommandBuffer> pCommandBuffer)
{
	std::cerr << "OpenGL: shouldn't call CopyDataTransferBufferWithinDevice on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CopyHostDataToDataTransferBuffer(void* pData, std::shared_ptr<DataTransferBuffer> pDstBuffer, size_t size)
{
	std::cerr << "OpenGL: shouldn't call CopyHostDataToDataTransferBuffer on OpenGL device.\n";
}

void DrawingDevice_OpenGL::CopyDataTransferBufferToHostDataLocation(std::shared_ptr<DataTransferBuffer> pSrcBuffer, void* pDataLoc)
{
	std::cerr << "OpenGL: shouldn't call CopyDataTransferBufferToHostDataLocation on OpenGL device.\n";
}

void DrawingDevice_OpenGL::SetPrimitiveTopology(GLenum mode)
{
	m_primitiveTopologyMode = mode;
}