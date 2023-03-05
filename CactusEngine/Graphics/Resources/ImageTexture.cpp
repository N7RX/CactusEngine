#include "ImageTexture.h"
#include "GraphicsDevice.h"
#include "GraphicsApplication.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Engine
{
	ImageTexture::ImageTexture(const char* filePath)
		: Texture2D(ETexture2DSource::ImageTexture)
	{
		LoadAndCreateTexture(filePath);
	}

	void ImageTexture::LoadAndCreateTexture(const char* filePath)
	{
		m_pDevice = ((GraphicsApplication*)gpGlobal->GetCurrentApplication())->GetGraphicsDevice();

		if (!m_pDevice)
		{
			throw std::runtime_error("Device is not assigned.");
			return;
		}

		int texWidth, texHeight, texChannels;
		stbi_uc* imageData = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!imageData)
		{
			throw std::runtime_error("Failed to load texture image.");
		}

		m_width = texWidth;
		m_height = texHeight;

		Texture2DCreateInfo createInfo{};
		createInfo.textureWidth = texWidth;
		createInfo.textureHeight = texHeight;
		createInfo.pTextureData = imageData;
		createInfo.dataType = EDataType::UByte;
		createInfo.format = ETextureFormat::RGBA8_SRGB; // Alert: this format might not be universal
		createInfo.textureType = ETextureType::SampledImage;
		createInfo.generateMipmap = true;
		createInfo.initialLayout = EImageLayout::ShaderReadOnly;

		m_pDevice->CreateTexture2D(createInfo, m_pTextureImpl);

		m_filePath.assign(filePath);
		stbi_image_free(imageData);
	}

	Texture2D* ImageTexture::GetTexture() const
	{
		return m_pTextureImpl;
	}

	bool ImageTexture::HasSampler() const
	{
		return m_pTextureImpl->HasSampler();
	}

	void ImageTexture::SetSampler(const TextureSampler* pSampler)
	{
		m_pTextureImpl->SetSampler(pSampler);
	}

	TextureSampler* ImageTexture::GetSampler() const
	{
		return m_pTextureImpl->GetSampler();
	}
}