#include "ImGuiOverlay.h"
#include "Timer.h"
#include "GraphicsApplication.h"
#include "ECSSceneReader.h"

namespace Engine
{
	void InitImGui(GLFWwindow* window)
	{
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType() == EGraphicsAPIType::OpenGL)
		{
			const char* glsl_version = "#version 130";

			ImGui::SetCurrentContext(ImGui::CreateContext());
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init(glsl_version);
		}
		else
		{

		}
	}

	void DestroyImGui()
	{
		LOG_WARNING("DestroyImGui not implemented.");
	}

	void DrawImGui()
	{
		ImGui_ImplOpenGL3_NewFrame();
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
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}