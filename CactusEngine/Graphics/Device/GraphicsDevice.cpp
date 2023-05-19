#include "GraphicsDevice.h"

namespace Engine
{
	GraphicsDevice::GraphicsDevice()
	{

	}

	GraphicsDevice::~GraphicsDevice()
	{
		ShutDown();
	}

	void GraphicsDevice::Initialize()
	{
		CreateDefaultSamplers();
	}

	void GraphicsDevice::ShutDown()
	{
		for (auto& pSampler : m_DefaultSamplers)
		{
			CE_DELETE(pSampler.second);
		}
	}

	TextureSampler* GraphicsDevice::GetTextureSampler(ESamplerAnisotropyLevel level)
	{
		if (m_DefaultSamplers.find(level) != m_DefaultSamplers.end())
		{
			return m_DefaultSamplers.at(level);
		}

		DEBUG_ASSERT_CE(level == ESamplerAnisotropyLevel::AFx2 || level == ESamplerAnisotropyLevel::AFx4 || level == ESamplerAnisotropyLevel::AFx8 || level == ESamplerAnisotropyLevel::AFx16);

		TextureSamplerCreateInfo createInfo{};
		createInfo.magMode = ESamplerFilterMode::Linear;
		createInfo.minMode = ESamplerFilterMode::Linear;
		createInfo.addressMode = ESamplerAddressMode::Repeat;
		createInfo.enableAnisotropy = true; // Non-AF sampler should already exsit
		createInfo.maxAnisotropy = (float)level;
		createInfo.enableCompareOp = false;
		createInfo.compareOp = ECompareOperation::Always;
		createInfo.mipmapMode = ESamplerMipmapMode::Linear;
		createInfo.minLod = 0;
		createInfo.maxLod = 12.0f; // Support up to 4096
		createInfo.minLodBias = 0;

		TextureSampler* pSampler = nullptr;
		CreateSampler(createInfo, pSampler);
		m_DefaultSamplers[level] = pSampler;

		return pSampler;
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
		createInfo.maxLod = 12.0f;
		createInfo.minLodBias = 0;

		TextureSampler* pSamplerNoAF = nullptr;
		CreateSampler(createInfo, pSamplerNoAF);

		m_DefaultSamplers[ESamplerAnisotropyLevel::None] = pSamplerNoAF;
	}
}
