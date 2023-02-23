#pragma once
#include "SharedTypes.h"
#include <cstdint>

namespace Engine
{
	class BaseConfiguration
	{
	public:
		BaseConfiguration() = default;
		virtual ~BaseConfiguration() = default;
	};

	class AppConfiguration : public BaseConfiguration
	{
	public:
		AppConfiguration() { m_appName = "CEApplication"; }

		void SetAppName(const char* appName)
		{
			m_appName = appName;
		}

		const char* GetAppName() const
		{
			return m_appName;
		}

	private:
		const char* m_appName;
	};

	class GraphicsConfiguration : public BaseConfiguration
	{
	public:
		GraphicsConfiguration()
		{
			m_graphicsAPIType = EGraphicsAPIType::Vulkan;
			m_preferredGPU = EGPUType::Discrete;
			m_windowWidth = 800;
			m_windowHeight = 600;
			m_enableVSync = false;
		}

		void SetGraphicsAPIType(EGraphicsAPIType type)
		{
			m_graphicsAPIType = type;
		}

		EGraphicsAPIType GetGraphicsAPIType() const
		{
			return m_graphicsAPIType;
		}

		void SetPreferredGPUType(EGPUType type)
		{
			m_preferredGPU = type;
		}

		EGPUType GetPreferredGPUType() const
		{
			return m_preferredGPU;
		}

		void SetWindowSize(uint32_t width, uint32_t height)
		{
			m_windowWidth = width;
			m_windowHeight = height;
		}

		uint32_t GetWindowWidth() const
		{
			return m_windowWidth;
		}

		uint32_t GetWindowHeight() const
		{
			return m_windowHeight;
		}

		float GetWindowAspect() const
		{
			return float(m_windowWidth) / float(m_windowHeight);
		}

		void SetVSync(bool val)
		{
			m_enableVSync = val;
		}

		bool GetVSync() const
		{
			return m_enableVSync;
		}

	private:
		EGraphicsAPIType m_graphicsAPIType;
		EGPUType m_preferredGPU;
		uint32_t m_windowWidth;
		uint32_t m_windowHeight;
		bool m_enableVSync;
	};
}