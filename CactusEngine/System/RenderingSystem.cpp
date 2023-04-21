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
		m_pDevice(nullptr),
		m_frameIndex(0),
		m_maxFramesInFlight(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight()),
		m_activeRenderer(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetActiveRenderer()),
		m_pendingResolutionUpdate(false),
		m_pauseRendering(false)
	{
		CreateDevice();
		RegisterRenderers();

		m_shaderPrograms.resize((uint32_t)EBuiltInShaderProgramType::COUNT, nullptr);
	}

	void RenderingSystem::Initialize()
	{
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetPrebuildShadersAndPipelines())
		{
			LoadAllShaders();
		}

		InitializeActiveRenderer();
	}

	void RenderingSystem::ShutDown()
	{

	}

	void RenderingSystem::FrameBegin()
	{
		if (m_pendingResolutionUpdate)
		{
			UpdateResolutionImpl();
			m_pendingResolutionUpdate = false;
		}
	}

	void RenderingSystem::Tick()
	{
		if (m_pauseRendering)
		{
			return;
		}

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

	ShaderProgram* RenderingSystem::GetShaderProgramByType(EBuiltInShaderProgramType type)
	{
		DEBUG_ASSERT_CE((uint32_t)type < m_shaderPrograms.size());

		if (m_shaderPrograms[(uint32_t)type] == nullptr)
		{
			bool result = LoadShader(type);
			DEBUG_ASSERT_CE(result);
		}

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
			// TODO: initialize new renderer resources and tear down old resources
		}
		else
		{
			LOG_ERROR("RenderingSystem: Trying to set a renderer that is not registered");
		}
	}

	void RenderingSystem::UpdateResolution()
	{
		m_pendingResolutionUpdate = true;
		// We need to wait until rendering is finished because resources might still be in use
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

	void RenderingSystem::InitializeActiveRenderer()
	{
		m_rendererTable[(uint32_t)m_activeRenderer]->BuildRenderGraph();
	}

	void RenderingSystem::LoadAllShaders()
	{
		LoadShader(EBuiltInShaderProgramType::Basic);
		LoadShader(EBuiltInShaderProgramType::Basic_Transparent);
		LoadShader(EBuiltInShaderProgramType::WaterBasic);
		LoadShader(EBuiltInShaderProgramType::DepthBased_ColorBlend_2);
		LoadShader(EBuiltInShaderProgramType::GBuffer);
		LoadShader(EBuiltInShaderProgramType::AnimeStyle);
		LoadShader(EBuiltInShaderProgramType::ShadowMap);
		LoadShader(EBuiltInShaderProgramType::DOF);
		LoadShader(EBuiltInShaderProgramType::DeferredLighting);
		LoadShader(EBuiltInShaderProgramType::DeferredLighting_Directional);
	}

	bool RenderingSystem::LoadShader(EBuiltInShaderProgramType type)
	{
		std::lock_guard<std::mutex> guard(m_shaderProgramsMutex);

		if (m_shaderPrograms[(uint32_t)type] != nullptr)
		{
			return true;
		}

		switch (m_pDevice->GetGraphicsAPIType())
		{
		case EGraphicsAPIType::OpenGL:
		{
			switch (type)
			{
			case EBuiltInShaderProgramType::Basic:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_OPENGL);
				break;

			case EBuiltInShaderProgramType::Basic_Transparent:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_TRANSPARENT_OPENGL);
				break;

			case EBuiltInShaderProgramType::WaterBasic:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_OPENGL);
				break;

			case EBuiltInShaderProgramType::DepthBased_ColorBlend_2:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2_OPENGL);
				break;

			case EBuiltInShaderProgramType::GBuffer:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_GBUFFER_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_GBUFFER_OPENGL);
				break;

			case EBuiltInShaderProgramType::AnimeStyle:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_OPENGL);
				break;

			case EBuiltInShaderProgramType::ShadowMap:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_SHADOWMAP_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_SHADOWMAP_OPENGL);
				break;

			case EBuiltInShaderProgramType::DOF:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_OF_FIELD_OPENGL);
				break;

			case EBuiltInShaderProgramType::DeferredLighting:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_DEFERRED_LIGHTING_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_OPENGL);
				break;

			case EBuiltInShaderProgramType::DeferredLighting_Directional:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_OPENGL, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_DIR_OPENGL);
				break;

			default:
			{
				throw std::runtime_error("Unhandled built-in shader type.");
				return false;
			}
			}
			return true;
		}
		case EGraphicsAPIType::Vulkan:
		{
			switch (type)
			{
			case EBuiltInShaderProgramType::Basic:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_VK);
				break;

			case EBuiltInShaderProgramType::Basic_Transparent:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_BASIC_TRANSPARENT_VK, BuiltInResourcesPath::SHADER_FRAGMENT_BASIC_TRANSPARENT_VK);
				break;

			case EBuiltInShaderProgramType::WaterBasic:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_WATER_BASIC_VK, BuiltInResourcesPath::SHADER_FRAGMENT_WATER_BASIC_VK);
				break;

			case EBuiltInShaderProgramType::DepthBased_ColorBlend_2:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_COLORBLEND_2_VK);
				break;

			case EBuiltInShaderProgramType::GBuffer:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_GBUFFER_VK, BuiltInResourcesPath::SHADER_FRAGMENT_GBUFFER_VK);
				break;

			case EBuiltInShaderProgramType::AnimeStyle:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_ANIMESTYLE_VK, BuiltInResourcesPath::SHADER_FRAGMENT_ANIMESTYLE_VK);
				break;

			case EBuiltInShaderProgramType::ShadowMap:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_SHADOWMAP_VK, BuiltInResourcesPath::SHADER_FRAGMENT_SHADOWMAP_VK);
				break;

			case EBuiltInShaderProgramType::DOF:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEPTH_OF_FIELD_VK);
				break;

			case EBuiltInShaderProgramType::DeferredLighting:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_DEFERRED_LIGHTING_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_VK);
				break;

			case EBuiltInShaderProgramType::DeferredLighting_Directional:
				m_shaderPrograms[(uint32_t)type] = m_pDevice->CreateShaderProgramFromFile(BuiltInResourcesPath::SHADER_VERTEX_FULLSCREEN_QUAD_VK, BuiltInResourcesPath::SHADER_FRAGMENT_DEFERRED_LIGHTING_DIR_VK);
				break;

			default:
			{
				throw std::runtime_error("Unhandled built-in shader type.");
				return false;
			}
			}
			return true;
		}
		default:
			throw std::runtime_error("Unhandled graphics device type.");
			return false;
		}
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
		auto pRenderer = m_rendererTable[(uint32_t)m_activeRenderer];

		if (!m_opaqueDrawList.empty() || !m_transparentDrawList.empty() || !m_lightDrawList.empty())
		{
			DEBUG_ASSERT_CE(pRenderer);

			// Estimate max draw call count

			uint32_t maxEstimatedDrawCall = 0;
			{
				uint32_t estimatedDrawCall = 0;
				for (auto& pEntity : m_opaqueDrawList)
				{
					estimatedDrawCall += pEntity->EstimateMaxDrawCallCount();
				}
				maxEstimatedDrawCall = std::max<uint32_t>(maxEstimatedDrawCall, estimatedDrawCall);

				estimatedDrawCall = 0;
				for (auto& pEntity : m_transparentDrawList)
				{
					estimatedDrawCall += pEntity->EstimateMaxDrawCallCount();
				}
				maxEstimatedDrawCall = std::max<uint32_t>(maxEstimatedDrawCall, estimatedDrawCall);

				estimatedDrawCall = 0;
				for (auto& pEntity : m_lightDrawList)
				{
					estimatedDrawCall += pEntity->EstimateMaxDrawCallCount();
				}
				maxEstimatedDrawCall = std::max<uint32_t>(maxEstimatedDrawCall, estimatedDrawCall);
			}

			pRenderer->UpdateMaxDrawCallCount(maxEstimatedDrawCall);

			// Fill context

			RenderContext context{};
			context.pOpaqueDrawList = &m_opaqueDrawList;
			context.pTransparentDrawList = &m_transparentDrawList;
			context.pLightDrawList = &m_lightDrawList;
			context.pCamera = pCamera;

			pRenderer->Draw(context, m_frameIndex);
		}
	}

	void RenderingSystem::UpdateResolutionImpl()
	{
		uint32_t width = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
		uint32_t height = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();

		// Window is minimized
		if (width == 0 && height == 0)
		{
			m_pauseRendering = true;
			return;
		}
		else
		{
			m_pauseRendering = false;
		}

		m_pDevice->WaitIdle();

		m_pDevice->ResizeSwapchain(width, height);

		m_rendererTable[(uint32_t)m_activeRenderer]->UpdateResolution(width, height);
	}
}