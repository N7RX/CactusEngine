#pragma once
#include "IRenderer.h"
#include "ECSWorld.h"
#include "GraphicsDevice.h"
#include "Global.h"
#include "BuiltInShaderType.h"
#include "NoCopy.h"

namespace Engine
{
	typedef std::unordered_map<ERendererType, std::shared_ptr<IRenderer>> RendererTable;
	typedef std::unordered_map<ERendererType, std::vector<std::shared_ptr<IEntity>>> RenderTaskTable;

	class RenderingSystem : public ISystem, std::enable_shared_from_this<RenderingSystem>, public NoCopy
	{
	public:
		RenderingSystem(ECSWorld* pWorld);
		~RenderingSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

		EGraphicsAPIType GetGraphicsAPIType() const;
		std::shared_ptr<ShaderProgram> GetShaderProgramByType(EBuiltInShaderProgramType type) const;

		template<typename T>
		inline void RegisterRenderer(ERendererType type, uint32_t priority = 0)
		{
			auto pRenderer = std::make_shared<T>(m_pDevice, this); // TODO: replace this temp solution to reference current System
			pRenderer->SetRendererPriority(priority);
			m_rendererTable.emplace(type, pRenderer);

			std::vector<std::shared_ptr<IEntity>> list;
			m_renderTaskTable.emplace(type, list);
		}

		void RemoveRenderer(ERendererType type);

	private:
		bool CreateDevice();
		bool RegisterRenderers();
		bool LoadShaders();
		void BuildRenderGraphs();
		void BuildRenderTask();
		void ExecuteRenderTask();

	private:
		uint32_t m_systemID;
		ECSWorld* m_pECSWorld;
		std::shared_ptr<GraphicsDevice> m_pDevice;
		std::vector<std::shared_ptr<ShaderProgram>> m_shaderPrograms;

		RendererTable	m_rendererTable;
		RenderTaskTable m_renderTaskTable;
	};
}