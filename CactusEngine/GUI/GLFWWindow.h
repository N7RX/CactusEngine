#pragma once
#include "BaseWindow.h"
#include "OpenGLIncludes.h" // Needs to be included before GLFW include

#include <GLFW/glfw3.h>

namespace Engine
{
	class GLFWWindow_CE : public BaseWindow
	{
	public:
		GLFWWindow_CE(const char* name, uint32_t width, uint32_t height);
		~GLFWWindow_CE();

		void Initialize() override;
		void Tick() override;
		void ShutDown() override;

		void RegisterCallback() override;

		void* GetWindowHandle() const override;

		void SetWindowSize(uint32_t width, uint32_t height) override;

	private:
		GLFWwindow* m_pGLFWWindowHandle;
	};

	extern void GLFWFramebufferSizeCallback_GL(GLFWwindow* pWindow, int32_t width, int32_t height);
	extern void GLFWFramebufferSizeCallback_VK(GLFWwindow* pWindow, int32_t width, int32_t height);
}