#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

namespace Engine
{
	void InitImGui(GLFWwindow* window);
	void DrawImGui();
	void DestroyImGui();
}