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
		GraphicsApplication() : m_pSetupFunc(nullptr) {};
		~GraphicsApplication() = default;

		void Initialize() override;
		void Tick() override;
		void ShutDown() override;

		bool ShouldQuit() const override;

		std::shared_ptr<ECSWorld> GetECSWorld() const;
		std::shared_ptr<GraphicsDevice> GetGraphicsDevice() const;
		std::shared_ptr<BaseWindow> GetWindow() const;
		void* GetWindowHandle() const override;

		void SetGraphicsDevice(const std::shared_ptr<GraphicsDevice> pDevice);
		void AddSetupFunction(void(*pSetupFunc)(GraphicsApplication* pApp));

	private:
		void InitWindow();
		void InitECS();

	private:
		std::shared_ptr<ECSWorld> m_pECSWorld;
		std::shared_ptr<GraphicsDevice> m_pDevice;
#if defined(GLFW_IMPLEMENTATION_CE)
		std::shared_ptr<GLFWWindow> m_pWindow;
#endif
		void(*m_pSetupFunc)(GraphicsApplication* pApp);
	};
}