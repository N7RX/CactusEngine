#pragma once
#include "BaseSystem.h"
#include "Configuration.h"
#include "ECSWorld.h"
#include "BasicMathTypes.h"

#if defined(GLFW_IMPLEMENTATION_CE)
#include <GLFW/glfw3.h>
#endif

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