#include "RenderingSystem.h"
#include "GraphicsApplication.h"
#include "BuiltInResourcesPath.h"
#include "LogUtility.h"
#include "RenderGraph.h"
#include "StandardRenderer.h"
#include "SimpleRenderer.h"
#include "AdvancedRenderer.h"
#include "GraphicsHardwareInterface_VK.h"
#include "GraphicsHardwareInterface_D3D12.h"
#include "MeshFilterComponent.h"
#include "MaterialComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"

namespace Engine
{
	RenderingSystem::RenderingSystem(ECSWorld* pWorld)
		: m_pECSWorld(pWorld),
		m_pDevice(nullptr),
		m_isRunning(true),
		m_activeRenderer(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetActiveRenderer()),
		m_rendererTable{},
		m_frameIndex(0),
		m_maxFramesInFlight(gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetMaxFramesInFlight()),
		m_pendingResolutionUpdate(false),
		m_pauseRendering(false)
	{
		m_shaderPrograms.resize((uint32_t)EBuiltInShaderProgramType::COUNT, nullptr);
	}

	void RenderingSystem::Initialize()
	{
		CreateDevice();
		
		if (gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetPrebuildShadersAndPipelines())
		{
			LoadAllShaders();
		}

		RegisterRenderers();
		InitializeActiveRenderer();

		m_renderThread = std::thread(&RenderingSystem::RenderThreadFunction, this);
	}

	void RenderingSystem::ShutDown()
	{
		m_isRunning = false;
		m_renderThread.join();
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

		{
			std::unique_lock<std::mutex> lock(m_renderThreadMutex);
			m_renderPhaseQueue.Push(ERenderPhaseType::Tick);
		}
		m_renderThreadCv.notify_one();
	}

	void RenderingSystem::FrameEnd()
	{
		if (m_pauseRendering)
		{
			return;
		}

		{
			std::unique_lock<std::mutex> lock(m_renderThreadMutex);
			m_renderPhaseQueue.Push(ERenderPhaseType::FrameEnd);
		}
		m_renderThreadCv.notify_one();
	}

	void RenderingSystem::WaitUntilFinish()
	{
		if (m_pauseRendering || !m_isRunning)
		{
			return;
		}

		m_renderFinishSemaphore.Wait();
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
		case EGraphicsAPIType::Vulkan:
			m_pDevice = CreateGraphicsDevice<EGraphicsAPIType::Vulkan>();
			break;
		case EGraphicsAPIType::D3D12:
			m_pDevice = CreateGraphicsDevice<EGraphicsAPIType::D3D12>();
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
		case EGraphicsAPIType::D3D12:
		{
			switch (type)
			{
			default:
			{
				throw std::runtime_error("D3D12 device is not implemented.");
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

	void RenderingSystem::RenderThreadFunction()
	{
		while (m_isRunning)
		{
			ERenderPhaseType phase = ERenderPhaseType::Invalid;

			{
				std::unique_lock<std::mutex> lock(m_renderThreadMutex);
				m_renderThreadCv.wait(lock, [this]() { return !m_renderPhaseQueue.Empty(); });
				m_renderPhaseQueue.TryPop(phase);
			}

			switch (phase)
			{
			case ERenderPhaseType::Tick:
			{
				BuildRenderTask();
				ExecuteRenderTask();

				break;
			}
			case ERenderPhaseType::FrameEnd:
			{
				m_opaqueDrawList.clear();
				m_transparentDrawList.clear();
				m_lightDrawList.clear();

				m_pDevice->Present(m_frameIndex);
				m_frameIndex = (m_frameIndex + 1) % m_maxFramesInFlight;

				m_renderFinishSemaphore.Signal();

				break;
			}
			default:
			{
				LOG_ERROR("Unhandled render phase type.");
				break;
			}
			}

			std::this_thread::yield();
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