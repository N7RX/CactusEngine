#pragma once
#include "BaseSystem.h"
#include "ECSWorld.h"
#include "Global.h"
#include "GLFWWindow.h"
#include "BasicMathTypes.h"

namespace Engine
{
	class InputSystem : public BaseSystem
	{
	public:
		InputSystem(ECSWorld* pWorld);
		~InputSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

		static bool GetKeyPress(char key);
		static bool GetMousePress(int key);
		static Vector2 GetCursorPosition();

	private:
#if defined(GLFW_IMPLEMENTATION_CE)
		static GLFWwindow* m_pGLFWWindow;
#endif
	};
}