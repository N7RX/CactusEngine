#pragma once

namespace Engine
{
	class DrawingCommandBuffer
	{
	public:
		unsigned int m_debugID = 0;

	protected:
		DrawingCommandBuffer() = default;
	};

	class DrawingCommandPool
	{
	protected:
		DrawingCommandPool() = default;
	};

	class DrawingSemaphore
	{
	protected:
		DrawingSemaphore() = default;
	};
}