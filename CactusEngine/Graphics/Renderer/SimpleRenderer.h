#pragma once
#include "BaseRenderer.h"

namespace Engine
{
	class ImageTexture;
	class SimpleRenderer : public BaseRenderer
	{
	public:
		SimpleRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem);
		~SimpleRenderer();

		void BuildRenderGraph() override;

	private:
		void BuildDummyResources();

	private:
		uint32_t m_renderThreadsCount;

		ImageTexture* m_pDummyInputTexture;
	};
}