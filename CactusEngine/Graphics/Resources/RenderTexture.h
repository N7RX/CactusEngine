#pragma once
#include "DrawingResources.h"

namespace Engine
{
	class DrawingDevice;
	class RenderTexture : public Texture2D
	{
	public:
		RenderTexture(uint32_t width, uint32_t height);
		~RenderTexture() = default;

		void FlushData(const void* pData, EDataType dataType, ETextureFormat format);

		std::shared_ptr<Texture2D> GetTexture() const;

		bool HasSampler() const override;
		void SetSampler(const std::shared_ptr<TextureSampler> pSampler) override;
		std::shared_ptr<TextureSampler> GetSampler() const override;

	private:
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::shared_ptr<Texture2D> m_pTextureImpl;
	};
}