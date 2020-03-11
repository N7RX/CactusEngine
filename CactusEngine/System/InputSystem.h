#pragma once
#include "ISystem.h"
#include "ECSWorld.h"
#include "Global.h"
#include "GLFWWindow.h"
#include "NoCopy.h"
#include "BasicMathTypes.h"

namespace Engine
{
	class InputSystem : public ISystem, public NoCopy
	{
	public:
		InputSystem(ECSWorld* pWorld);
		~InputSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

		static bool GetKeyPress(char key);
		static bool GetMousePress(int key);
		static Vector2 GetCursorPosition();

	private:
		uint32_t m_systemID;
#if defined(GLFW_IMPLEMENTATION_CE)
		static GLFWwindow* m_pGLFWWindow;
#endif
	};
}