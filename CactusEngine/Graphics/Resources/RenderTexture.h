#pragma once
#include "GraphicsResources.h"

namespace Engine
{
	class RenderTexture : public Texture2D
	{
	public:
		RenderTexture(uint32_t width, uint32_t height);
		~RenderTexture() = default;

		void FlushData(const void* pData, EDataType dataType, ETextureFormat format);

		Texture2D* GetTexture() const;

		bool HasSampler() const override;
		void SetSampler(const TextureSampler* pSampler) override;
		TextureSampler* GetSampler() const override;

	private:
		GraphicsDevice* m_pDevice;
		Texture2D* m_pTextureImpl;
	};
}