#pragma once
#include "DrawingResources.h"

namespace Engine
{
	class RenderTexture : public Texture2D
	{
	public:
		RenderTexture(uint32_t width, uint32_t height);
		~RenderTexture() = default;
	};
}