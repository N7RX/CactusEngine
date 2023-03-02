#pragma once
#include "GraphicsResources.h"

namespace Engine
{
	class ImageTexture : public Texture2D
	{
	public:
		ImageTexture(const char* filePath);
		~ImageTexture() = default;

		Texture2D* GetTexture() const;

		bool HasSampler() const override;
		void SetSampler(const TextureSampler* pSampler) override;
		TextureSampler* GetSampler() const override;

	private:
		void LoadAndCreateTexture(const char* filePath);

	private:
		GraphicsDevice* m_pDevice;
		Texture2D* m_pTextureImpl;
	};
}