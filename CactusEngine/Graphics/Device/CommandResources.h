#pragma once

namespace Engine
{
	class DrawingCommandBuffer
	{
	public:
		uint32_t m_debugID = 0;

	protected:
		DrawingCommandBuffer() = default;
	};

	class DrawingCommandPool
	{
	protected:
		DrawingCommandPool() = default;
	};
}