#pragma once
#include "GraphicsResources.h"

namespace Engine
{
	class GraphicsDevice;
	class ImageTexture : public Texture2D
	{
	public:
		ImageTexture(const char* filePath);
		~ImageTexture() = default;

		std::shared_ptr<Texture2D> GetTexture() const;

		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	private:
		void LoadAndCreateTexture(const char* filePath);

	private:
		std::shared_ptr<GraphicsDevice> m_pDevice;
		std::shared_ptr<Texture2D> m_pTextureImpl;
	};
}