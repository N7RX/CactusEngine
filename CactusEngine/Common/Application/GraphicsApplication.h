#pragma once
#include "BaseApplication.h"
#include "ECSWorld.h"
#include "DrawingDevice.h"
#if defined(GLFW_IMPLEMENTATION_CACTUS)
#include "GLFWWindow.h"
#endif

namespace Engine
{
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
		std::shared_ptr<DrawingDevice> GetDrawingDevice() const;
		std::shared_ptr<BaseWindow> GetWindow() const;
		void* GetWindowHandle() const override;

		void SetDrawingDevice(const std::shared_ptr<DrawingDevice> pDevice);
		void AddSetupFunction(void(*pSetupFunc)(GraphicsApplication* pApp));

	private:
		void InitWindow();
		void InitECS();

	private:
		std::shared_ptr<ECSWorld> m_pECSWorld;
		std::shared_ptr<DrawingDevice> m_pDevice;
#if defined(GLFW_IMPLEMENTATION_CACTUS)
		std::shared_ptr<GLFWWindow> m_pWindow;
#endif
		void(*m_pSetupFunc)(GraphicsApplication* pApp);
	};
}