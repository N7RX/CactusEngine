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
	glGenerateMipmap(GL_TEXTURE_2D);
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

bool DrawingDevice_OpenGL::CreateVertexBuffer(const VertexBufferCreateInfo& createInfo, std::shared_ptr<VertexBuffer>& pOutput)
{
	std::shared_ptr<VertexBuffer_OpenGL> pVertexBuffer = std::make_shared<VertexBuffer_OpenGL>();
	
	glGenVertexArrays(1, &pVertexBuffer->m_vao);
	glBindVertexArray(pVertexBuffer->m_vao);

	// Index
	glGenBuffers(1, &pVertexBuffer->m_vboIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pVertexBuffer->m_vboIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * createInfo.indexDataCount, createInfo.pIndexData, GL_STATIC_DRAW);

	// Position
	glGenBuffers(1, &pVertexBuffer->m_vboPositions);
	glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboPositions);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * createInfo.positionDataCount, createInfo.pPositionData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_POSITION_LOCATION);
	glVertexAttribPointer(ATTRIB_POSITION_LOCATION, 3, GL_FLOAT, 0, 0, 0);

	// Normal
	glGenBuffers(1, &pVertexBuffer->m_vboNormals);
	glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboNormals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * createInfo.normalDataCount, createInfo.pNormalData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_NORMAL_LOCATION);
	glVertexAttribPointer(ATTRIB_NORMAL_LOCATION, 3, GL_FLOAT, 0, 0, 0);

	// TexCoord
	glGenBuffers(1, &pVertexBuffer->m_vboTexcoords);
	glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboTexcoords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * createInfo.texcoordDataCount, createInfo.pTexcoordData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD_LOCATION);
	glVertexAttribPointer(ATTRIB_TEXCOORD_LOCATION, 2, GL_FLOAT, 0, 0, 0);

	// Tangent
	glGenBuffers(1, &pVertexBuffer->m_vboTangents);
	glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboTangents);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * createInfo.tangentDataCount, createInfo.pTangentData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_TANGENT_LOCATION);
	glVertexAttribPointer(ATTRIB_TANGENT_LOCATION, 3, GL_FLOAT, 0, 0, 0);

	// Bitangent
	glGenBuffers(1, &pVertexBuffer->m_vboBitangents);
	glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->m_vboBitangents);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * createInfo.bitangentDataCount, createInfo.pBitangentData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTRIB_BITANGENT_LOCATION);
	glVertexAttribPointer(ATTRIB_BITANGENT_LOCATION, 3, GL_FLOAT, 0, 0, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	pVertexBuffer->SetNumberOfIndices(createInfo.indexDataCount);
	pVertexBuffer->SetData(nullptr, static_cast<uint32_t>(sizeof(int) * createInfo.indexDataCount + sizeof(float) * createInfo.positionDataCount + sizeof(float) * createInfo.normalDataCount + sizeof(float) * createInfo.texcoordDataCount));

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
	if (createInfo.format == eFormat_Depth)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	pTexture->SetGLTextureID(texID);
	pTexture->MarkTextureSize(createInfo.textureWidth, createInfo.textureHeight);
	pTexture->SetTextureType(createInfo.textureType);
	pTexture->SetData(nullptr, static_cast<uint32_t>(createInfo.textureWidth * createInfo.textureHeight * OpenGLTypeSize(createInfo.dataType)));

	return true;
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
	for (int i = 0; i < createInfo.bindTextures.size(); ++i)
	{
		switch (createInfo.bindTextures[i]->GetTextureType())
		{
		case eTextureType_ColorAttachment:
			pFrameBuffer->AddColorAttachment(GL_COLOR_ATTACHMENT0 + colorAttachmentCount); // Alert: this could cause the framebuffer to be partially "initialized" when creation failed
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachmentCount, GL_TEXTURE_2D, createInfo.bindTextures[i]->GetTextureID(), 0);
			colorAttachmentCount++;
			break;
		case eTextureType_DepthAttachment:
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, createInfo.bindTextures[i]->GetTextureID(), 0);
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

	return true;
}

void DrawingDevice_OpenGL::ClearRenderTarget()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		glDrawBuffers(pTarget->GetColorAttachmentCount(), pTarget->GetColorAttachments());
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
			glDrawBuffers(colorAttachments.size(), colorAttachments.data());
		}
	}
}

void DrawingDevice_OpenGL::SetClearColor(Color4 color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void DrawingDevice_OpenGL::SetBlendState(const DeviceBlendStateInfo& blendInfo)
{
	if (blendInfo.enabled)
	{
		glEnable(GL_BLEND);
		glBlendFunc(OpenGLBlendFactor(blendInfo.srcFactor), OpenGLBlendFactor(blendInfo.dstFactor));
	}
	else
	{
		glDisable(GL_BLEND);
	}
}

void DrawingDevice_OpenGL::UpdateShaderParameter(std::shared_ptr<ShaderProgram> pShaderProgram, const std::shared_ptr<ShaderParameterTable> pTable)
{
	auto pProgram = std::static_pointer_cast<ShaderProgram_OpenGL>(pShaderProgram);

	glUseProgram(pProgram->GetGLProgramID());

	for (auto& entry : pTable->m_table)
	{
		pProgram->UpdateParameterValue(entry.location, entry.type, entry.value);
	}
}

void DrawingDevice_OpenGL::SetVertexBuffer(const std::shared_ptr<VertexBuffer> pVertexBuffer)
{
	if (!pVertexBuffer)
	{
		return;
	}

	glBindVertexArray(std::static_pointer_cast<VertexBuffer_OpenGL>(pVertexBuffer)->m_vao);
}

void DrawingDevice_OpenGL::DrawPrimitive(uint32_t indicesCount, uint32_t baseIndex, uint32_t baseVertex)
{
	glDrawElementsBaseVertex(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int)*baseIndex), baseVertex);
}

void DrawingDevice_OpenGL::DrawFullScreenQuad()
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

void DrawingDevice_OpenGL::Present()
{

}

EGraphicsDeviceType DrawingDevice_OpenGL::GetDeviceType() const
{
	return eDevice_OpenGL;
}

void DrawingDevice_OpenGL::ConfigureStates_Test()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}