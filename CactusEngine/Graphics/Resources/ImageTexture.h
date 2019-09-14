#pragma once
#include "DrawingResources.h"

namespace Engine
{
	class DrawingDevice;
	class ImageTexture : public Texture2D
	{
	public:
		ImageTexture(const char* filePath);
		~ImageTexture() = default;

		std::shared_ptr<Texture2D> GetTexture() const;
		uint32_t GetTextureID() const override;

	private:
		void LoadAndCreateTexture(const char* filePath);

	private:
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::shared_ptr<Texture2D> m_pTextureImpl;
	};
}