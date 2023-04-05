#include "RenderingSystem.h"
#include "GraphicsApplication.h"
#include "BuiltInResourcesPath.h"
#include "LogUtility.h"
#include "RenderGraph.h"
#include "StandardRenderer.h"
#include "SimpleRenderer.h"
#include "AdvancedRenderer.h"
#include "GraphicsHardwareInterface_GL.h"
#include "GraphicsHardwareInterface_VK.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"

namespace Engine
{
	RenderingSystem::RenderingSystem(ECSWorld* pWorld)
		: m_pECSWorld(pWorld),
		m_frameIndex(0),
		m_maxFramesInFlight(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight())
	{
		m_activeRenderer = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetActiveRenderer();

		CreateDevice();
		RegisterRenderers();

		m_shaderPrograms.resize((uint32_t)EBuiltInShaderProgramType::COUNT);
	}

	void RenderingSystem::Initialize()
	{
		LoadShaders();
		InitializeActiveRenderer();
	}

	void RenderingSystem::ShutDown()
	{

	}

	void RenderingSystem::FrameBegin()
	{

	}

	void RenderingSystem::Tick()
	{
		BuildRenderTask();
		ExecuteRenderTask();
	}

	void RenderingSystem::FrameEnd()
	{
		m_opaqueDrawList.clear();
		m_transparentDrawList.clear();
		m_lightDrawList.clear();

		m_pDevice->Present(m_frameIndex);

		m_frameIndex = (m_frameIndex + 1) % m_maxFramesInFlight;
	}

	EGraphicsAPIType RenderingSystem::GetGraphicsAPIType() const
	{
		return m_pDevice->GetGraphicsAPIType();
	}

	ShaderProgram* RenderingSystem::GetShaderProgramByType(EBuiltInShaderProgramType type) const
	{
		DEBUG_ASSERT_CE((uint32_t)type < m_shaderPrograms.size());
		return m_shaderPrograms[(uint32_t)type];
	}

	void RenderingSystem::RemoveRenderer(ERendererType type)
	{
		DEBUG_ASSERT_CE((uint32_t)type < (uint32_t)ERendererType::COUNT);
		DEBUG_ASSERT_MESSAGE_CE(m_rendererTable[(uint32_t)type], "Trying to remove a renderer that is not loaded.");

		//... Unload renderer

		m_rendererTable[(uint32_t)type] = nullptr;
	}

	void RenderingSystem::SetActiveRenderer(ERendererType type)
	{
		DEBUG_ASSERT_CE((uint32_t)type < (uint32_t)ERendererType::COUNT);
		if (m_rendererTable[(uint32_t)type])
		{
			m_activeRenderer = type;
		}
		else
		{
			LOG_ERROR("RenderingSystem: Trying to set a renderer that is not registered");
		}
	}

	bool RenderingSystem::CreateDevice()
	{
		switch (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetGraphicsAPIType())
		{
		case EGraphicsAPIType::OpenGL:
			m_pDevice = CreateGraphicsDevice<EGraphicsAPIType::OpenGL>();
			break;
		case EGraphicsAPIType::Vulkan:
			m_pDevice = CreateGraphicsDevice<EGraphicsAPIType::Vulkan>();
			break;
		default:
			throw std::runtime_error("Unsupported graphics device type.");
			return false;
		}

		m_pDevice->Initialize();
		((GraphicsApplication*)gpGlobal->GetCurrentApplication())->SetGraphicsDevice(m_pDevice);

		return true;
	}

	void RenderingSystem::RegisterRenderers()
	{
		RegisterRenderer<SimpleRenderer>(ERendererType::Simple, 1);
		RegisterRenderer<StandardRenderer>(ERendererType::Standard, 0);
		RegisterRenderer<AdvancedRenderer>(ERendererType::Advanced, 2);
	}

	bool RenderingSystem::LoadShaders()
	{
		switch (m_pDevice->GetGraphicsAPIType())
		{
		case EGraphicsAPIType::OpenGL:
		{
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic_Transparent] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_TRANSPARENT_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::WaterBasic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DepthBased_ColorBlend_2] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::GBuffer] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_GBUFFER_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_GBUFFER_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::AnimeStyle] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::ShadowMap] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_SHADOWMAP_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_SHADOWMAP_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DOF] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_OF_FIELD_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DeferredLighting] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_DEFERRED_LIGHTING_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_OPENGL);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DeferredLighting_Directional] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_DIR_OPENGL);
			break;
		}
		case EGraphicsAPIType::Vulkan:
		{
			// TODO: load pipeline cache instead
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::Basic_Transparent] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_TRANSPARENT_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_TRANSPARENT_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::WaterBasic] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DepthBased_ColorBlend_2] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::GBuffer] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_GBUFFER_VK, BuiltInResourcesPath::SHADER_FRAGMENT_GBUFFER_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::AnimeStyle] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_VK, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::ShadowMap] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_SHADOWMAP_VK, BuiltInResourcesPath::SHADER_FRAGMENT_SHADOWMAP_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DOF] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_OF_FIELD_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DeferredLighting] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_DEFERRED_LIGHTING_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_VK);
			m_shaderPrograms[(uint32_t)EBuiltInShaderProgramType::DeferredLighting_Directional] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_DIR_VK);
			break;
		}
		default:
			throw std::runtime_error("Unhandled graphics device type.");
			return false;
		}
		return true;
	}

	void RenderingSystem::InitializeActiveRenderer()
	{
		m_rendererTable[(uint32_t)m_activeRenderer]->BuildRenderGraph();
	}

	void RenderingSystem::BuildRenderTask()
	{
		auto pEntityList = m_pECSWorld->GetEntityList();
		for (auto itr = pEntityList->begin(); itr != pEntityList->end(); ++itr)
		{
			auto pMeshFilterComp = (MeshFilterComponent*)itr->second->GetComponent(EComponentType::MeshFilter);
			auto pMaterialComp = (MaterialComponent*)itr->second->GetComponent(EComponentType::Material);
			auto pLightComp = (LightComponent*)itr->second->GetComponent(EComponentType::Light);

			if (pMeshFilterComp && pMaterialComp)
			{
				if (pMaterialComp->HasTransparency())
				{
					m_transparentDrawList.emplace_back(itr->second);
					// TODO: sort transparency
				}
				// In case of partial transparency, we need to add it to opaque list as well. This might be optimized later
				m_opaqueDrawList.emplace_back(itr->second);
			}
			if (pLightComp)
			{
				m_lightDrawList.emplace_back(itr->second);
			}
		}
	}

	void RenderingSystem::ExecuteRenderTask()
	{
		auto pCamera = m_pECSWorld->FindEntityWithTag(EEntityTag::MainCamera);

		if (!m_opaqueDrawList.empty() || !m_transparentDrawList.empty() || !m_lightDrawList.empty())
		{
			DEBUG_ASSERT_CE(m_rendererTable[(uint32_t)m_activeRenderer]);

			RenderContext context{};
			context.pOpaqueDrawList = &m_opaqueDrawList;
			context.pTransparentDrawList = &m_transparentDrawList;
			context.pLightDrawList = &m_lightDrawList;
			context.pCamera = pCamera;

			m_rendererTable[(uint32_t)m_activeRenderer]->Draw(context, m_frameIndex);
		}
	}
}