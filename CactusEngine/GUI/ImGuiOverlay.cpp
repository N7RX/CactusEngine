#include "ImGuiOverlay.h"
#include "Timer.h"

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
	ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "FPS: %u", Timer::GetAverageFPS());
	ImGui::End();

	ImGui::Render();
}