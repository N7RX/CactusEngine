#include "ImGuiOverlay.h"
#include "Timer.h"
#include "GraphicsApplication.h"
#include "Global.h"
#include "ECSSceneReader.h"

using namespace Engine;

void Engine::InitImGui(GLFWwindow* window)
{
	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGui_ImplGlfwGL3_Init(window, true);
}

void Engine::DrawImGui()
{
	ImGui_ImplGlfwGL3_NewFrame();

	ImGui::Begin("Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "FPS: %u", Timer::GetAverageFPS());
	ImGui::End();

	ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::Button("Unity Chan"))
	{
		auto pWorld = std::static_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetECSWorld();
		pWorld->ClearEntities();
		ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Lucy"))
	{
		auto pWorld = std::static_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetECSWorld();
		pWorld->ClearEntities();
		ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Serapis"))
	{
		auto pWorld = std::static_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->GetECSWorld();
		pWorld->ClearEntities();
		ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");
	}
	ImGui::End();

	ImGui::Render();
}