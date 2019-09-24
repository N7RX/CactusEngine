#include "InputSystem.h"
#include "Timer.h"
#include <iostream>

using namespace Engine;

InputSystem::InputSystem(ECSWorld* pWorld)
{
#if defined(GLFW_IMPLEMENTATION_CACTUS)
	m_pGLFWWindow = reinterpret_cast<GLFWwindow*>(gpGlobal->GetWindowHandle());
#endif
}

void InputSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t InputSystem::GetSystemID() const
{
	return m_systemID;
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
#if defined(GLFW_IMPLEMENTATION_CACTUS)
	assert(m_pGLFWWindow != nullptr);

	if (glfwGetKey(m_pGLFWWindow, GLFW_KEY_F) == GLFW_PRESS)
	{
		std::cout << "Pressed F\n";
	}

	// Do sth.
#endif
}

void InputSystem::FrameEnd()
{

}