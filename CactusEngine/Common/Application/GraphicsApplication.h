#pragma once
#include "BaseApplication.h"

namespace Engine
{
	class ECSWorld;
	class BaseWindow;
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

		BaseWindow* m_pWindow;

		void(*m_pSetupFunc)(GraphicsApplication* pApp);
	};
}