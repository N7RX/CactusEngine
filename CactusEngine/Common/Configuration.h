#pragma once
#include "SharedTypes.h"

#include <cstdint>

// Macro configurations

#if defined(_DEBUG)
#define DEBUG_MODE_CE
#endif

#define DEVELOPMENT_MODE_CE


#define PLATFORM_WINDOWS_CE

#if defined(PLATFORM_WINDOWS_CE)
#define GLFW_IMPLEMENTATION_CE
#endif

// Runtime configurations

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
		AppConfiguration()
			: m_appName("CE_Application"),
			m_appVersion("1.0.0"),
			m_engineVersion("0.1.5")
		{

		}

		void SetAppName(const char* appName)
		{
			m_appName = appName;
		}

		const char* GetAppName() const
		{
			return m_appName;
		}

		void SetAppVersion(const char* version)
		{
			m_appVersion = version;
		}

		const char* GetAppVersion() const
		{
			return m_appVersion;
		}

		const char* GetEngineVersion() const
		{
			return m_engineVersion;
		}

	private:
		const char* m_appName;
		const char* m_appVersion;
		const char* m_engineVersion;
	};

	class GraphicsConfiguration : public BaseConfiguration
	{
	public:
		GraphicsConfiguration()
			: m_graphicsAPIType(EGraphicsAPIType::Vulkan),
			m_preferredGPU(EGPUType::Discrete),
			m_windowWidth(1280),
			m_windowHeight(720),
			m_maxFramesInFlight(3),
			m_enableVSync(false),
			m_prebuildShadersAndPipelines(true),
			m_samplerAnisotropyLevel(ESamplerAnisotropyLevel::None),
			m_activeRenderer(ERendererType::Standard),
			m_renderScale(1.0f)
		{

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

		// Right now this can only be set before render system initializes
		void SetMaxFramesInFlight(uint32_t val)
		{
			if (val > 3)
			{
				val = 3;
			}
			else if (val == 0)
			{
				val = 1;
			}
			// Allowed values: 1, 2, 3
			m_maxFramesInFlight = val;
		}

		uint32_t GetMaxFramesInFlight() const
		{
			return m_maxFramesInFlight;
		}

		void SetVSync(bool val)
		{
			m_enableVSync = val;
		}

		bool GetVSync() const
		{
			return m_enableVSync;
		}

		void SetPrebuildShadersAndPipelines(bool val)
		{
			m_prebuildShadersAndPipelines = val;
		}

		bool GetPrebuildShadersAndPipelines() const
		{
			return m_prebuildShadersAndPipelines;
		}

		void SetActiveRenderer(ERendererType type)
		{
			m_activeRenderer = type;
		}

		ERendererType GetActiveRenderer() const
		{
			return m_activeRenderer;
		}

		void SetTextureAnisotropyLevel(ESamplerAnisotropyLevel level)
		{
			m_samplerAnisotropyLevel = level;
		}

		ESamplerAnisotropyLevel GetTextureAnisotropyLevel() const
		{
			return m_samplerAnisotropyLevel;
		}

		void SetRenderScale(float scale)
		{
			if (scale > 2.0f)
			{
				scale = 2.0f;
			}
			else if (scale < 0.5f)
			{
				scale = 0.5f;
			}
			// Allowed range: 0.5 - 2.0
			m_renderScale = scale;
		}

		float GetRenderScale() const
		{
			return m_renderScale;
		}

	private:
		// Graphics API to use
		EGraphicsAPIType m_graphicsAPIType;

		// Select to use discrete or integrated GPU (Vulkan only)
		EGPUType m_preferredGPU;

		// Window size
		uint32_t m_windowWidth;
		uint32_t m_windowHeight;

		// How many frames CPU can buffer ahead of GPU
		// Higher value consumes more memory and creates more input latency, but may also reduce stuttering
		uint32_t m_maxFramesInFlight;

		// If true, vsync will be enabled
		bool m_enableVSync;

		// If true, shaders and pipelines will be prebuilt at startup, may result in longer startup time and higher memory usage
		// If false, they will be built on demand, may result in frame stuttering
		bool m_prebuildShadersAndPipelines;

		// Type of renderer currently being used
		ERendererType m_activeRenderer;

		// Global texture anisotropic filtering level
		ESamplerAnisotropyLevel m_samplerAnisotropyLevel;

		// Internal render resolution multiplier
		float m_renderScale;
	};
}