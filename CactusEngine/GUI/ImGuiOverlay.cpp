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

	ImGui::Begin("GLC", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Ray Origin (0, 0), ([1, 256], 0), (0, [1, 256])");
	ImGui::SliderInt("Ray-Gen Horizontal", &GLCDrawingSystem::m_sGLCOrigin_Hori, 1, 256);
	ImGui::SliderInt("Ray-Gen Vertical", &GLCDrawingSystem::m_sGLCOrigin_Vert, 1, 256);
	if (ImGui::Button("Perspective"))
	{
		GLCDrawingSystem::m_sGLCOrigin_Hori = 1;
		GLCDrawingSystem::m_sGLCOrigin_Vert = 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Orthographic"))
	{
		GLCDrawingSystem::m_sGLCOrigin_Hori = 256;
		GLCDrawingSystem::m_sGLCOrigin_Vert = 256;
	}
	ImGui::SameLine();
	if (ImGui::Button("Disassemble"))
	{
		GLCDrawingSystem::m_sGLCOrigin_Hori = 32;
		GLCDrawingSystem::m_sGLCOrigin_Vert = 32;
	}
	ImGui::ColorEdit3("Background Color", glm::value_ptr(GLCDrawingSystem::m_sBackgroundColor));
	ImGui::SliderFloat("Second Plane", &GLCComponent::m_sSecondImagePlane, 0.5f, 1.5f);
	ImGui::End();

	ImGui::Render();
}