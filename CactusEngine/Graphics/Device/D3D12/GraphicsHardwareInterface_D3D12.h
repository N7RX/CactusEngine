#pragma once
#include "GraphicsDevice.h"


namespace Engine
{
	template<>
	static GraphicsDevice* CreateGraphicsDevice<EGraphicsAPIType::D3D12>()
	{
		throw std::runtime_error("GraphicsDevice: D3D12 device is not implemented.");
		return nullptr;
	}
}