#include "DrawingSystem.h"
#include "ForwardRenderer.h"
#include "DrawingDevice_OpenGL.h"
#include "DrawingDevice_Vulkan.h"
#include "MeshRendererComponent.h"
#include "CameraComponent.h"
#include "GraphicsApplication.h"
#include "BuiltInResourcesPath.h"
#include <assert.h>

using namespace Engine;

DrawingSystem::DrawingSystem(ECSWorld* pWorld)
	: m_pECSWorld(pWorld)
{
	CreateDevice();

	RegisterRenderer<ForwardRenderer>(eRenderer_Forward, 1);

	m_shaderPrograms.resize(EBUILTINSHADERTYPE_COUNT);
}

void DrawingSystem::SetSystemID(uint32_t id)
{
	m_systemID = id;
}

uint32_t DrawingSystem::GetSystemID() const
{
	return m_systemID;
}

void DrawingSystem::Initialize()
{
	LoadShaders();
	BuildRenderGraphs();
}

void DrawingSystem::ShutDown()
{

}

void DrawingSystem::FrameBegin()
{

}

void DrawingSystem::Tick()
{
	BuildRenderTask();
	ConfigureRenderEnvironment();
	ExecuteRenderTask();
}

void DrawingSystem::FrameEnd()
{
	for (auto& renderList : m_renderTaskTable)
	{
		renderList.second.clear();
	}
}

EGraphicsDeviceType DrawingSystem::GetDeviceType() const
{
	return m_pDevice->GetDeviceType();
}

std::shared_ptr<ShaderProgram> DrawingSystem::GetShaderProgramByType(EBuiltInShaderProgramType type) const
{
	assert(type < m_shaderPrograms.size());
	return m_shaderPrograms[type];
}

void DrawingSystem::RemoveRenderer(ERendererType type)
{
	m_rendererTable.erase(type);
	m_renderTaskTable.erase(type);
}

bool DrawingSystem::CreateDevice()
{
	switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(eConfiguration_Graphics)->GetDeviceType())
	{
	case eDevice_OpenGL:
		m_pDevice = CreateDrawingDevice<eDevice_OpenGL>();
		break;
	case eDevice_Vulkan:
		m_pDevice = CreateDrawingDevice<eDevice_Vulkan>();
		break;
	default:
		throw std::runtime_error("Unsupported drawing device type.");
	}

	m_pDevice->Initialize();
	std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->SetDrawingDevice(m_pDevice);

	return true;
}

bool DrawingSystem::LoadShaders()
{
	switch (m_pDevice->GetDeviceType())
	{
	case eDevice_OpenGL:
	{
		m_shaderPrograms[eShaderProgram_Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_OPENGL);
		m_shaderPrograms[eShaderProgram_WaterBasic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_OPENGL);
		m_shaderPrograms[eShaderProgram_DepthBased_ColorBlend_2] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2);
		m_shaderPrograms[eShaderProgram_NormalOnly] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_NORMALONLY_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_NORMALONLY_OPENGL);
		m_shaderPrograms[eShaderProgram_GaussianBlur] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_GAUSSIANBLUR);
		m_shaderPrograms[eShaderProgram_AnimeStyle] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_OPENGL);
		m_shaderPrograms[eShaderProgram_LineDrawing_Curvature] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_CURVATURE);
		m_shaderPrograms[eShaderProgram_LineDrawing_Color] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_COLOR);
		m_shaderPrograms[eShaderProgram_LineDrawing_Blend] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_BLEND);
		break;
	}
	case eDevice_Vulkan:
		break;
	}
	return true;
}

void DrawingSystem::BuildRenderGraphs()
{
	for (auto& pRenderer : m_rendererTable)
	{
		pRenderer.second->BuildRenderGraph();
	}
}

void DrawingSystem::BuildRenderTask()
{
	const EntityList* pEntityList = m_pECSWorld->GetEntityList();
	for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
	{
		auto pMeshRendererComp = std::static_pointer_cast<MeshRendererComponent>(itr->second->GetComponent(eCompType_MeshRenderer));
		if (pMeshRendererComp)
		{
			m_renderTaskTable.at(pMeshRendererComp->GetRendererType()).emplace_back(itr->second);
		}
	}
}

void DrawingSystem::ConfigureRenderEnvironment()
{
	m_pDevice->ConfigureStates_Test();
}

void DrawingSystem::ExecuteRenderTask()
{
	auto pCamera = m_pECSWorld->FindEntityWithTag(eEntityTag_MainCamera);
	auto pCameraComp = std::static_pointer_cast<CameraComponent>(pCamera->GetComponent(eCompType_Camera));

	if (pCameraComp)
	{
		m_pDevice->SetClearColor(pCameraComp->GetClearColor());
	}
	m_pDevice->ClearRenderTarget();

	// Alert: we are ignoring renderer priority at this moment
	for (auto& renderList : m_renderTaskTable)
	{
		if (renderList.second.size() > 0)
		{
			m_rendererTable.at(renderList.first)->Draw(renderList.second, pCamera);
		}
	}

	m_pDevice->Present();
}