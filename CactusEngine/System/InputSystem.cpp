#include "InputSystem.h"
#include "Timer.h"

#include <iostream>

using namespace Engine;

GLFWwindow* InputSystem::m_pGLFWWindow = nullptr;

InputSystem::InputSystem(ECSWorld* pWorld)
{
#if defined(GLFW_IMPLEMENTATION_CE)
	m_pGLFWWindow = reinterpret_cast<GLFWwindow*>(gpGlobal->GetWindowHandle());
#endif
}

void InputSystem::Initialize()
{

}

void InputSystem::ShutDown()
{

}

void InputSystem::FrameBegin()
{

}

void InputSystem::Tick()
{
	// TODO: remove this workaround
	static bool pressedF = false;
	if (glfwGetKey(m_pGLFWWindow, GLFW_KEY_F) == GLFW_PRESS)
	{
		pressedF = true;
	}
	else if (pressedF && glfwGetKey(m_pGLFWWindow, GLFW_KEY_F) == GLFW_RELEASE)
	{
		std::cout << "FPS: " << Timer::GetAverageFPS() << std::endl;
		pressedF = false;
	}
}

void InputSystem::FrameEnd()
{

}

bool InputSystem::GetKeyPress(char key)
{
#if defined(GLFW_IMPLEMENTATION_CE)
	assert(m_pGLFWWindow != nullptr);

	if (glfwGetKey(m_pGLFWWindow, GLFW_KEY_A + (int)(key - 'a')) == GLFW_PRESS)
	{
		return true;
	}

	return false;
#endif
}

bool InputSystem::GetMousePress(int key)
{
#if defined(GLFW_IMPLEMENTATION_CE)
	assert(m_pGLFWWindow != nullptr);

	if (glfwGetMouseButton(m_pGLFWWindow, GLFW_MOUSE_BUTTON_1 + key) == GLFW_PRESS)
	{
		return true;
	}

	return false;
#endif
}

Vector2 InputSystem::GetCursorPosition()
{
#if defined(GLFW_IMPLEMENTATION_CE)
	assert(m_pGLFWWindow != nullptr);

	double xPos, yPos;
	glfwGetCursorPos(m_pGLFWWindow, &xPos, &yPos);

	return Vector2(xPos, yPos);
#endif
}