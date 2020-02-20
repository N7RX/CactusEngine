#include "DrawingSystem.h"
#include "ForwardRenderer.h"
#include "HPForwardRenderer.h"
#include "DrawingDevice_OpenGL.h"
#include "DrawingDevice_Vulkan.h"
#include "MeshRendererComponent.h"
#include "CameraComponent.h"
#include "GraphicsApplication.h"
#include "BuiltInResourcesPath.h"
#include <assert.h>

#define ENABLE_HETEROGENEOUS_GPUS_CE

using namespace Engine;

DrawingSystem::DrawingSystem(ECSWorld* pWorld)
	: m_pECSWorld(pWorld)
{
	CreateDevice();
	RegisterRenderers();

	m_shaderPrograms.resize((uint32_t)EBuiltInShaderProgramType::COUNT);
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
	assert((uint32_t)type < m_shaderPrograms.size());
	return m_shaderPrograms[(uint32_t)type];
}

void DrawingSystem::RemoveRenderer(ERendererType type)
{
	m_rendererTable.erase(type);
	m_renderTaskTable.erase(type);
}

bool DrawingSystem::CreateDevice()
{
	switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType())
	{
	case EGraphicsDeviceType::OpenGL:
		m_pDevice = CreateDrawingDevice<EGraphicsDeviceType::OpenGL>();
		break;
	case EGraphicsDeviceType::Vulkan:
		m_pDevice = CreateDrawingDevice<EGraphicsDeviceType::Vulkan>();
		break;
	default:
		throw std::runtime_error("Unsupported drawing device type.");
		return false;
	}

	m_pDevice->Initialize();
	std::dynamic_pointer_cast<GraphicsApplication>(gpGlobal->GetCurrentApplication())->SetDrawingDevice(m_pDevice);

	return true;
}

bool DrawingSystem::RegisterRenderers()
{
	switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetDeviceType())
	{
	case EGraphicsDeviceType::OpenGL:
		RegisterRenderer<ForwardRenderer>(ERendererType::Forward, 1);
		break;
	case EGraphicsDeviceType::Vulkan:
		RegisterRenderer<HPForwardRenderer>(ERendererType::Forward, 1);
		break;
	default:
		throw std::runtime_error("Unsupported drawing device type.");
		return false;
	}

	return true;
}

bool DrawingSystem::LoadShaders()
{
	switch (m_pDevice->GetDeviceType())
	{
	case EGraphicsDeviceType::OpenGL:
	{
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic_Transparent] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_TRANSPARENT_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::WaterBasic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DepthBased_ColorBlend_2] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::NormalOnly] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_NORMALONLY_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_NORMALONLY_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::GaussianBlur] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_GAUSSIANBLUR_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::AnimeStyle] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::LineDrawing_Curvature] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_CURVATURE_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::LineDrawing_Color] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_COLOR_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::LineDrawing_Blend] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_LINEDRAWING_BLEND_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::ShadowMap] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_SHADOWMAP_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_SHADOWMAP_OPENGL);
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DOF] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_OF_FIELD_OPENGL);
		break;
	}
	case EGraphicsDeviceType::Vulkan:
#if defined(ENABLE_HETEROGENEOUS_GPUS_CE)
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_VK, EGPUType::Discrete);

#else
		m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_VK);

#endif
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
	auto pEntityList = m_pECSWorld->GetEntityList();
	for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
	{
		auto pMeshRendererComp = std::static_pointer_cast<MeshRendererComponent>(itr->second->GetComponent(EComponentType::MeshRenderer));
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
	auto pCamera = m_pECSWorld->FindEntityWithTag(EEntityTag::MainCamera);
	auto pCameraComp = pCamera ? std::static_pointer_cast<CameraComponent>(pCamera->GetComponent(EComponentType::Camera)) : nullptr;

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

	//m_pDevice->Present();
}