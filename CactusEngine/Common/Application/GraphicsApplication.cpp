#include "GraphicsApplication.h"
#include "ECSWorld.h"
#include "Timer.h"
#include "MemoryAllocator.h"
#if defined(GLFW_IMPLEMENTATION_CE)
#include "GLFWWindow.h"
#endif

namespace Engine
{
	GraphicsApplication::GraphicsApplication()
		: m_pECSWorld(nullptr),
		m_pDevice(nullptr),
		m_pWindow(nullptr),
		m_pSetupFunc(nullptr)
	{

	}

	void GraphicsApplication::Initialize()
	{
		CE_NEW(m_pECSWorld, ECSWorld);

		InitWindow(); // Because we are binding the init of GLAD with GLFW, this has to be done before InitECS()

		if (m_pSetupFunc)
		{
			m_pSetupFunc(this);
		}

		InitECS();

		m_pWindow->SetRenderingSystem((RenderingSystem*)(m_pECSWorld->GetSystem(ESystemType::Rendering)));

		Timer::Initialize();
	}

	void GraphicsApplication::Tick()
	{
		Timer::FrameBegin();

		m_pECSWorld->Tick();
		m_pWindow->Tick();

		Timer::FrameEnd();
	}

	void GraphicsApplication::ShutDown()
	{
		m_pECSWorld->ShutDown();
		m_pWindow->ShutDown();
	}

	bool GraphicsApplication::ShouldQuit() const
	{
		return (m_shouldQuit || m_pWindow->ShouldQuit());
	}

	ECSWorld* GraphicsApplication::GetECSWorld() const
	{
		return m_pECSWorld;
	}

	GraphicsDevice* GraphicsApplication::GetGraphicsDevice() const
	{
		return m_pDevice;
	}

	BaseWindow* GraphicsApplication::GetWindow() const
	{
		return m_pWindow;
	}

	void* GraphicsApplication::GetWindowHandle() const
	{
		return m_pWindow->GetWindowHandle();
	}

	void GraphicsApplication::SetGraphicsDevice(GraphicsDevice* pDevice)
	{
		m_pDevice = pDevice;
	}

	void GraphicsApplication::AddSetupFunction(void(*pSetupFunc)(GraphicsApplication * pApp))
	{
		m_pSetupFunc = pSetupFunc;
	}

	void GraphicsApplication::InitWindow()
	{
#if defined(GLFW_IMPLEMENTATION_CE)
		CE_NEW(m_pWindow, GLFWWindow_CE,
			gpGlobal->GetConfiguration<AppConfiguration>(EConfigurationType::App)->GetAppName(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth(),
			gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight()
			);
#endif
		if (!m_pWindow)
		{
			throw std::runtime_error("Failed to create window.");
		}

		m_pWindow->Initialize();
	}

	void GraphicsApplication::InitECS()
	{
		m_pECSWorld->Initialize();
	}
}