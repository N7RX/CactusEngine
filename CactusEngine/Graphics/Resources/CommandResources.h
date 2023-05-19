#pragma once

namespace Engine
{
	// Graphics command buffer itself should not provide any functions as it may be empty
	// It should only be passed around as a handle in the render graph nodes
	class GraphicsCommandBuffer
	{
	public:
		unsigned int m_debugID = 0;

	protected:
		GraphicsCommandBuffer() = default;
	};

	class GraphicsCommandPool
	{
	protected:
		GraphicsCommandPool() = default;
	};

	class GraphicsSemaphore
	{
	protected:
		GraphicsSemaphore() = default;
	};
}