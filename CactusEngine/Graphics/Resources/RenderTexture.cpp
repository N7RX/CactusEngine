#include "RenderTexture.h"
#include "GraphicsDevice.h"
#include "GraphicsApplication.h"

namespace Engine
{
	RenderTexture::RenderTexture(uint32_t width, uint32_t height)
		: Texture2D(ETexture2DSource::RenderTexture),
		m_pDevice(((GraphicsApplication*)gpGlobal->GetCurrentApplication())->GetGraphicsDevice()),
		m_pTextureImpl(nullptr)
	{
		m_width = width;
		m_height = height;
	}

	void RenderTexture::FlushData(const void* pData, EDataType dataType, ETextureFormat format)
	{
		if (!m_pDevice)
		{
			return;
		}

		Texture2DCreateInfo createInfo{};
		createInfo.textureWidth = m_width;
		createInfo.textureHeight = m_height;
		createInfo.pTextureData = pData;
		createInfo.dataType = dataType;
		createInfo.format = format;
		createInfo.generateMipmap = false; // TODO: test out the effect
		createInfo.initialLayout = EImageLayout::ShaderReadOnly;

		m_pDevice->CreateTexture2D(createInfo, m_pTextureImpl);
	}

	Texture2D* RenderTexture::GetTexture() const
	{
		return m_pTextureImpl;
	}

	bool RenderTexture::HasSampler() const
	{
		return m_pTextureImpl->HasSampler();
	}

	void RenderTexture::SetSampler(const TextureSampler* pSampler)
	{
		m_pTextureImpl->SetSampler(pSampler);
	}

	TextureSampler* RenderTexture::GetSampler() const
	{
		return m_pTextureImpl->GetSampler();
	}
}