#pragma once
#include "BaseApplication.h"
#if defined(GLFW_IMPLEMENTATION_CE)
#include "GLFWWindow.h"
#endif

namespace Engine
{
	class ECSWorld;
	class GraphicsDevice;

	class GraphicsApplication : public BaseApplication
	{
	public:
		GraphicsApplication();
		~GraphicsApplication() = default;

		void Initialize() override;
		void Tick() override;
		void ShutDown() override;

		bool ShouldQuit() const override;

		ECSWorld* GetECSWorld() const;
		GraphicsDevice* GetGraphicsDevice() const;
		BaseWindow* GetWindow() const;
		void* GetWindowHandle() const override;

		void SetGraphicsDevice(GraphicsDevice* pDevice);
		void AddSetupFunction(void(*pSetupFunc)(GraphicsApplication* pApp));

	private:
		void InitWindow();
		void InitECS();

	private:
		ECSWorld* m_pECSWorld;
		GraphicsDevice* m_pDevice;
#if defined(GLFW_IMPLEMENTATION_CE)
		GLFWWindow* m_pWindow;
#endif
		void(*m_pSetupFunc)(GraphicsApplication* pApp);
	};
}