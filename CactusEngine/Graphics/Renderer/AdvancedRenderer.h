#pragma once
#include "BaseRenderer.h"

namespace Engine
{
	class AdvancedRenderer : public BaseRenderer
	{
	public:
		AdvancedRenderer(GraphicsDevice* pDevice, RenderingSystem* pSystem);
		~AdvancedRenderer() = default;

		void BuildRenderGraph() override;
	};
}