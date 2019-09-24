#include "RenderTexture.h"
#include "DrawingDevice.h"
#include "Global.h"
#include "GraphicsApplication.h"

using namespace Engine;

RenderTexture::RenderTexture(uint32_t width, uint32_t height)
{
	m_pDevice = std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetDrawingDevice();

	m_width = width;
	m_height = height;
}

void RenderTexture::FlushData(const void* pData, EDataType dataType, ETextureFormat format)
{
	if (!m_pDevice)
	{
		return;
	}

	Texture2DCreateInfo createInfo = {};
	createInfo.textureWidth = m_width;
	createInfo.textureHeight = m_height;
	createInfo.pTextureData = pData;
	createInfo.dataType = dataType;
	createInfo.format = format;

	m_pDevice->CreateTexture2D(createInfo, m_pTextureImpl);
}

std::shared_ptr<Texture2D> RenderTexture::GetTexture() const
{
	return m_pTextureImpl;
}

uint32_t RenderTexture::GetTextureID() const
{
	return m_pTextureImpl->GetTextureID();
}