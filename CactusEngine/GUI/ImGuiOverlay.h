#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw_gl3.h"

namespace Engine
{
	void InitImGui(GLFWwindow* window);
	void DrawImGui();
}