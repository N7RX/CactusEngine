#include "GraphicsApplication.h"
#include "ECSWorld.h"
#include "Timer.h"
#include "MemoryAllocator.h"
#if defined(GLFW_IMPLEMENTATION_CE)
#include "GLFWWindow.h"
#endif
#include "RenderingSystem.h"
#include "InputSystem.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"
#include "AudioSystem.h"

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

		InitSystems();

		InitWindow(); // Because part of platform initialization is done in GLFW, this has to be called before InitECS()

		auto pRenderingSystem = (RenderingSystem*)(m_pECSWorld->GetSystem(ESystemType::Rendering));
		DEBUG_ASSERT_CE(pRenderingSystem);
		m_pWindow->SetRenderingSystem(pRenderingSystem);
		pRenderingSystem->SetDeviceWindow(m_pWindow);

		InitECS();

		pRenderingSystem->AcquireRenderContextOwnership();

		if (m_pSetupFunc)
		{
			m_pSetupFunc(this);
		}

		pRenderingSystem->ReleaseRenderContextOwnership();

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

	void GraphicsApplication::InitSystems()
	{
		m_pECSWorld->RegisterSystem<RenderingSystem>(ESystemType::Rendering, 2);
		m_pECSWorld->RegisterSystem<AnimationSystem>(ESystemType::Animation, 1);
		m_pECSWorld->RegisterSystem<InputSystem>(ESystemType::Input, 1);
		m_pECSWorld->RegisterSystem<ScriptSystem>(ESystemType::Script, 0);
		m_pECSWorld->RegisterSystem<AudioSystem>(ESystemType::Audio, 3); // Right now this is only for performance simulation
		m_pECSWorld->SortSystems();
	}

	void GraphicsApplication::InitECS()
	{
		m_pECSWorld->Initialize();
	}
}