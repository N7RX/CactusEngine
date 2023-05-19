#include "ImGuiOverlay.h"
#include "Timer.h"
#include "GraphicsApplication.h"
#include "ECSSceneReader.h"

namespace Engine
{
	void InitImGui(GLFWwindow* window)
	{
		LOG_ERROR("InitImGui not implemented.");
	}

	void DestroyImGui()
	{
		LOG_WARNING("DestroyImGui not implemented.");
	}

	void DrawImGui()
	{
		//ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "FPS: %u", Timer::GetAverageFPS());
		ImGui::End();

		ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (ImGui::Button("Unity Chan"))
		{
			auto pWorld = ((GraphicsApplication*)gpGlobal->GetCurrentApplication())->GetECSWorld();
			pWorld->ClearEntities();
			ReadECSWorldFromJson(pWorld, "Assets/Scene/UnityChanScene.json");
		}
		ImGui::SameLine();
		if (ImGui::Button("Lucy"))
		{
			auto pWorld = ((GraphicsApplication*)gpGlobal->GetCurrentApplication())->GetECSWorld();
			pWorld->ClearEntities();
			ReadECSWorldFromJson(pWorld, "Assets/Scene/LucyScene.json");
		}
		ImGui::SameLine();
		if (ImGui::Button("Serapis"))
		{
			auto pWorld = ((GraphicsApplication*)gpGlobal->GetCurrentApplication())->GetECSWorld();
			pWorld->ClearEntities();
			ReadECSWorldFromJson(pWorld, "Assets/Scene/SerapisScene.json");
		}
		ImGui::End();

		ImGui::Render();
		//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData());
	}
}