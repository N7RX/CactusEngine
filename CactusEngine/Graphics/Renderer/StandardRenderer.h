#pragma once
#include "BaseRenderer.h"

namespace Engine
{
	class StandardRenderer : public BaseRenderer
	{
	public:
		StandardRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem);
		~StandardRenderer() = default;

		void BuildRenderGraph() override;
	};
}