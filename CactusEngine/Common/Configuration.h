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
			m_deviceType = EGraphicsDeviceType::eDevice_OpenGL;
			m_windowWidth = 800;
			m_windowHeight = 600;
			m_enableVSync = false;
		}

		void SetDeviceType(EGraphicsDeviceType type)
		{
			m_deviceType = type;
		}

		EGraphicsDeviceType GetDeviceType() const
		{
			return m_deviceType;
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
		EGraphicsDeviceType m_deviceType;
		uint32_t m_windowWidth;
		uint32_t m_windowHeight;
		bool m_enableVSync;
	};
}