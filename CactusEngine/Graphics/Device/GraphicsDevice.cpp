#include "GraphicsDevice.h"

namespace Engine
{
	void GraphicsDevice::Initialize()
	{
		CreateDefaultSamplers();
	}

	void GraphicsDevice::ShutDown()
	{
		CE_DELETE(m_pDefaultSampler_NoAF);
		CE_DELETE(m_pDefaultSampler_4xAF);
		CE_DELETE(m_pDefaultSampler_8xAF);
		CE_DELETE(m_pDefaultSampler_16xAF);
	}

	TextureSampler* GraphicsDevice::GetTextureSampler(ESamplerAnisotropyLevel level) const
	{
		switch (level)
		{
		case ESamplerAnisotropyLevel::None:
			return m_pDefaultSampler_NoAF;
		case ESamplerAnisotropyLevel::AFx2:
			LOG_WARNING("2xAF sampler is unsupported, falling back to no AF.");
			return m_pDefaultSampler_NoAF;
		case ESamplerAnisotropyLevel::AFx4:
			return m_pDefaultSampler_4xAF;
		case ESamplerAnisotropyLevel::AFx8:
			return m_pDefaultSampler_8xAF;
		case ESamplerAnisotropyLevel::AFx16:
			return m_pDefaultSampler_16xAF;
		default:
			LOG_ERROR("Unsupported texture sampler anisotropy level.");
		}

		return nullptr;
	}

	void GraphicsDevice::CreateDefaultSamplers()
	{
		TextureSamplerCreateInfo createInfo{};
		createInfo.magMode = ESamplerFilterMode::Linear;
		createInfo.minMode = ESamplerFilterMode::Linear;
		createInfo.addressMode = ESamplerAddressMode::Repeat;
		createInfo.enableAnisotropy = false;
		createInfo.maxAnisotropy = 1;
		createInfo.enableCompareOp = false;
		createInfo.compareOp = ECompareOperation::Always;
		createInfo.mipmapMode = ESamplerMipmapMode::Linear;
		createInfo.minLod = 0;
		createInfo.maxLod = 12.0f; // Support up to 4096
		createInfo.minLodBias = 0;

		CreateSampler(createInfo, m_pDefaultSampler_NoAF);

		createInfo.enableAnisotropy = true;

		createInfo.maxAnisotropy = 4;
		CreateSampler(createInfo, m_pDefaultSampler_4xAF);

		createInfo.maxAnisotropy = 8;
		CreateSampler(createInfo, m_pDefaultSampler_8xAF);

		createInfo.maxAnisotropy = 16;
		CreateSampler(createInfo, m_pDefaultSampler_16xAF);
	}
}
