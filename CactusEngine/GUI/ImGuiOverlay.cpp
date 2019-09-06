#include "ImGuiOverlay.h"
#include "GLCDrawingSystem.h"

using namespace Engine;

void Engine::InitImGui(GLFWwindow* window)
{
	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGui_ImplGlfwGL3_Init(window, true);
}

void Engine::DrawImGui()
{
	ImGui_ImplGlfwGL3_NewFrame();

	ImGui::Begin("ImGui", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::End();

	ImGui::Render();
}