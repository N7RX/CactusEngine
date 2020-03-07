#include "GLFWWindow.h"
#include "Global.h"
#include "ImGuiOverlay.h"
#include <iostream>

using namespace Engine;

GLFWWindow::GLFWWindow(const char* name, uint32_t width, uint32_t height)
	: BaseWindow(name, width, height)
{
}

GLFWWindow::~GLFWWindow()
{
	ShutDown();
}

void Engine::GLFWFramebufferSizeCallback_GL(GLFWwindow* pWindow, int width, int height)
{
	glViewport(0, 0, width, height);
}

void Engine::GLFWFramebufferSizeCallback_VK(GLFWwindow* pWindow, int width, int height)
{
	std::cout << "Vulkan window resized but not handled\n";
}

void GLFWWindow::Initialize()
{
	glfwInit();

	switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType())
	{
	case EGraphicsDeviceType::OpenGL:
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_pGLFWWindowHandle = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowName, NULL, NULL);
		if (m_pGLFWWindowHandle == NULL)
		{
			throw std::runtime_error("Failed to create GLFW window");
			glfwTerminate();
		}
		glfwMakeContextCurrent(m_pGLFWWindowHandle);
		glfwSetFramebufferSizeCallback(m_pGLFWWindowHandle, GLFWFramebufferSizeCallback_GL);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::runtime_error("Failed to initialize GLAD");
		}

		gpGlobal->MarkGlobalState(EGlobalStateQueryType::GLFWInit, true);
		break;
	}
	case EGraphicsDeviceType::Vulkan:
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_pGLFWWindowHandle = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowName, NULL, NULL);
		if (m_pGLFWWindowHandle == NULL)
		{
			throw std::runtime_error("Failed to create GLFW window");
			glfwTerminate();
		}

		glfwSetFramebufferSizeCallback(m_pGLFWWindowHandle, GLFWFramebufferSizeCallback_VK);

		gpGlobal->MarkGlobalState(EGlobalStateQueryType::GLFWInit, true);
		break;
	}
	default:
		throw std::runtime_error("Unsupported device type when initializing GFLW window.");
		break;
	}

	if (!gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetVSync())
	{
		glfwSwapInterval(0);
	}

#if defined(_DEBUG) // To ensure same performance baseline before ImGui support is implemented for Vulkan device
	if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType() == EGraphicsDeviceType::OpenGL)
	{
		InitImGui(m_pGLFWWindowHandle);
	}
#endif
}

void GLFWWindow::Tick()
{
#if defined(_DEBUG)
	if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType() == EGraphicsDeviceType::OpenGL)
	{
		DrawImGui();
	}
#endif

	m_shouldQuit = glfwWindowShouldClose(m_pGLFWWindowHandle);
	glfwSwapBuffers(m_pGLFWWindowHandle);
	glfwPollEvents();
}

void GLFWWindow::ShutDown()
{
	glfwTerminate();
}

void GLFWWindow::RegisterCallback()
{

}

void* GLFWWindow::GetWindowHandle() const
{
	return m_pGLFWWindowHandle;
}

void GLFWWindow::SetWindowSize(uint32_t width, uint32_t height)
{
	BaseWindow::SetWindowSize(width, height);
	glViewport(0, 0, width, height);
}
