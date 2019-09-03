#include "BaseWindow.h"
#include "Global.h"

using namespace Engine;

BaseWindow::BaseWindow(const char* name, uint32_t width, uint32_t height)
	: m_windowName(name), m_windowWidth(width), m_windowHeight(height), m_shouldQuit(false)
{
}

void BaseWindow::SetWindowID(uint32_t id)
{
	m_windowID = id;
}

void BaseWindow::SetWindowName(const char* name)
{
	m_windowName = name;
}

void BaseWindow::SetWindowSize(uint32_t width, uint32_t height)
{
	m_windowWidth = width;
	m_windowHeight = height;
	gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->SetWindowSize(width, height);
	// This base function does not implement any actual resizing
}

uint32_t BaseWindow::GetWindowID() const
{
	return m_windowID;
}

const char* BaseWindow::GetWindowName() const
{
	return m_windowName;
}

uint32_t BaseWindow::GetWindowWidth() const
{
	return m_windowWidth;
}

uint32_t BaseWindow::SetWindowHeight() const
{
	return m_windowHeight;
}

bool BaseWindow::ShouldQuit() const
{
	return m_shouldQuit;
}