#include "GLFWWindow.h"
#include "Global.h"
#include "ImGuiOverlay.h"
#include "RenderingSystem.h"

namespace Engine
{
	static bool s_windowSizeUpdated = false;
	static uint32_t s_newWindowWidth = 0;
	static uint32_t s_newWindowHeight = 0;

	GLFWWindow_CE::GLFWWindow_CE(const char* name, uint32_t width, uint32_t height)
		: BaseWindow(name, width, height),
		m_pGLFWWindowHandle(nullptr)
	{

	}

	GLFWWindow_CE::~GLFWWindow_CE()
	{
		ShutDown();
	}

	void GLFWFramebufferSizeCallback(GLFWwindow* pWindow, int32_t width, int32_t height)
	{
		s_windowSizeUpdated = true;
		s_newWindowWidth = width;
		s_newWindowHeight = height;
	}

	void GLFWWindow_CE::Initialize()
	{
		glfwInit();

		switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType())
		{
		case EGraphicsAPIType::OpenGL:
		{
			// Using OpenGL 4.6
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			m_pGLFWWindowHandle = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowName, NULL, NULL);
			if (m_pGLFWWindowHandle == NULL)
			{
				throw std::runtime_error("Failed to create GLFW window");
				glfwTerminate();
			}
			glfwMakeContextCurrent(m_pGLFWWindowHandle);
			glfwSetFramebufferSizeCallback(m_pGLFWWindowHandle, GLFWFramebufferSizeCallback);

			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				throw std::runtime_error("Failed to initialize GLAD");
			}

			gpGlobal->MarkGlobalState(EGlobalStateQueryType::GLFWInit, true);
			break;
		}
		case EGraphicsAPIType::Vulkan:
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			m_pGLFWWindowHandle = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowName, NULL, NULL);
			if (m_pGLFWWindowHandle == NULL)
			{
				throw std::runtime_error("Failed to create GLFW window");
				glfwTerminate();
			}

			glfwSetFramebufferSizeCallback(m_pGLFWWindowHandle, GLFWFramebufferSizeCallback);

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

		// TODO: Vulkan backend support
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType() == EGraphicsAPIType::OpenGL)
		{
			InitImGui(m_pGLFWWindowHandle);
		}
	}

	void GLFWWindow_CE::Tick()
	{
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType() == EGraphicsAPIType::OpenGL)
		{
			DrawImGui();
			glfwSwapBuffers(m_pGLFWWindowHandle);
		}

		if (s_windowSizeUpdated)
		{
			SetWindowSize(s_newWindowWidth, s_newWindowHeight);
			UpdateWindowSizeInternal();
			s_windowSizeUpdated = false;
		}

		m_shouldQuit = glfwWindowShouldClose(m_pGLFWWindowHandle);

		glfwPollEvents();
	}

	void GLFWWindow_CE::ShutDown()
	{
		glfwTerminate();
	}

	void GLFWWindow_CE::RegisterCallback()
	{

	}

	void* GLFWWindow_CE::GetWindowHandle() const
	{
		return m_pGLFWWindowHandle;
	}

	void GLFWWindow_CE::SetWindowSize(uint32_t width, uint32_t height)
	{
		BaseWindow::SetWindowSize(width, height);

		s_windowSizeUpdated = true;
		s_newWindowWidth = width;
		s_newWindowHeight = height;
	}

	void GLFWWindow_CE::UpdateWindowSizeInternal()
	{
		DEBUG_ASSERT_CE(m_pRenderingSystem != nullptr);
		m_pRenderingSystem->UpdateResolution();
	}
}