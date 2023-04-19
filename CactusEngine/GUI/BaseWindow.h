#pragma once
#include "NoCopy.h"

#include <cstdint>

namespace Engine
{
	class RenderingSystem;

	class BaseWindow : public NoCopy
	{
	public:
		virtual ~BaseWindow() = default;

		virtual void Initialize() = 0;
		virtual void Tick() = 0;
		virtual void ShutDown() = 0;
		virtual void RegisterCallback() = 0;
		virtual void* GetWindowHandle() const = 0;

		void SetWindowID(uint32_t id);
		void SetWindowName(const char* name);
		virtual void SetWindowSize(uint32_t width, uint32_t height);

		uint32_t GetWindowID() const;
		const char* GetWindowName() const;
		uint32_t GetWindowWidth() const;
		uint32_t SetWindowHeight() const;

		virtual bool ShouldQuit() const;

		void SetRenderingSystem(RenderingSystem* pSystem);

	protected:
		BaseWindow() = default;
		BaseWindow(const char* name, uint32_t width, uint32_t height);

	protected:
		uint32_t m_windowID;
		const char* m_windowName;
		uint32_t m_windowWidth;
		uint32_t m_windowHeight;
		bool m_shouldQuit;

		RenderingSystem* m_pRenderingSystem;
	};
}