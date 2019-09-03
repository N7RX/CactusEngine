#pragma once
#include "BaseApplication.h"
#include "ECSWorld.h"
#include "DrawingDevice.h"
#include "GLFWWindow.h"

namespace Engine
{
	class GraphicsApplication : public BaseApplication
	{
	public:
		GraphicsApplication() = default;
		~GraphicsApplication() = default;

		void Initialize() override;
		void Tick() override;
		void ShutDown() override;

		bool ShouldQuit() const override;

		std::shared_ptr<ECSWorld> GetECSWorld() const;
		std::shared_ptr<DrawingDevice> GetDrawingDevice() const;

		void SetDrawingDevice(const std::shared_ptr<DrawingDevice> pDevice);
		void AddSetupFunction(void(*pSetupFunc)(GraphicsApplication* pApp));

	private:
		void InitWindow();
		void InitECS();

	private:
		std::shared_ptr<ECSWorld> m_pECSWorld;
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::shared_ptr<GLFWWindow> m_pWindow;
		void(*m_pSetupFunc)(GraphicsApplication* pApp);
	};
}