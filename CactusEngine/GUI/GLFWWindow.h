#pragma once
#include "BaseWindow.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"

namespace Engine
{
	class GLFWWindow : public BaseWindow
	{
	public:
		GLFWWindow(const char* name, uint32_t width, uint32_t height);
		~GLFWWindow();

		void Initialize() override;
		void Tick() override;
		void ShutDown() override;

		void RegisterCallback() override;

		void* GetWindowHandle() const override;
		GLFWwindow* GetGLFWWindowHandle() const;

		void SetWindowSize(uint32_t width, uint32_t height) override;

	private:
		GLFWwindow* m_pGLFWWindowHandle;
	};

	extern void GLFWFramebufferSizeCallback(GLFWwindow* pWindow, int width, int height);
}