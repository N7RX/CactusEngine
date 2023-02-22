#pragma once

namespace Engine
{
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