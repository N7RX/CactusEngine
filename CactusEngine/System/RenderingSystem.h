#pragma once
#include "BaseRenderer.h"
#include "ECSWorld.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	class ShaderProgram;

	typedef std::unordered_map<ERendererType, std::shared_ptr<BaseRenderer>> RendererTable;
	typedef std::unordered_map<ERendererType, std::vector<std::shared_ptr<BaseEntity>>> RenderTaskTable;

	class RenderingSystem : public BaseSystem, std::enable_shared_from_this<RenderingSystem>
	{
	public:
		RenderingSystem(ECSWorld* pWorld);
		~RenderingSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

		EGraphicsAPIType GetGraphicsAPIType() const;
		std::shared_ptr<ShaderProgram> GetShaderProgramByType(EBuiltInShaderProgramType type) const;

		template<typename T>
		inline void RegisterRenderer(ERendererType type, uint32_t priority = 0)
		{
			auto pRenderer = std::make_shared<T>(m_pDevice, this); // TODO: replace this temp solution to reference current System
			pRenderer->SetRendererPriority(priority);
			m_rendererTable.emplace(type, pRenderer);

			std::vector<std::shared_ptr<BaseEntity>> list;
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
		ECSWorld* m_pECSWorld;
		std::shared_ptr<GraphicsDevice> m_pDevice;
		std::vector<std::shared_ptr<ShaderProgram>> m_shaderPrograms;

		RendererTable	m_rendererTable;
		RenderTaskTable m_renderTaskTable;
	};
}