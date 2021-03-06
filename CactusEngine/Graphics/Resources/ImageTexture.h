#pragma once
#include "DrawingResources.h"

namespace Engine
{
	class DrawingDevice;
	class ImageTexture : public Texture2D
	{
	public:
		ImageTexture(const char* filePath, EGPUType deviceType = EGPUType::Main);
		~ImageTexture() = default;

		std::shared_ptr<Texture2D> GetTexture() const;

		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	private:
		void LoadAndCreateTexture(const char* filePath, EGPUType deviceType);

	private:
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::shared_ptr<Texture2D> m_pTextureImpl;
	};
}