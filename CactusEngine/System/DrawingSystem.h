#pragma once
#include "IRenderer.h"
#include "ECSWorld.h"
#include "DrawingDevice.h"
#include "Global.h"
#include "BuiltInShaderType.h"

namespace Engine
{
	typedef std::unordered_map<ERendererType, std::shared_ptr<IRenderer>> RendererTable;
	typedef std::unordered_map<ERendererType, std::vector<std::shared_ptr<IEntity>>> RenderTaskTable;

	class DrawingSystem : public ISystem, std::enable_shared_from_this<DrawingSystem>
	{
	public:
		DrawingSystem(ECSWorld* pWorld);
		~DrawingSystem() = default;

		void SetSystemID(uint32_t id);
		uint32_t GetSystemID() const;

		void Initialize();
		void ShutDown();

		void FrameBegin();
		void Tick();
		void FrameEnd();

		EGraphicsDeviceType GetDeviceType() const;
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
		bool LoadShaders();
		void BuildRenderTask();
		void ConfigureRenderEnvironment();
		void ExecuteRenderTask();

	private:
		uint32_t m_systemID;
		ECSWorld* m_pECSWorld;
		std::shared_ptr<DrawingDevice> m_pDevice;
		std::vector<std::shared_ptr<ShaderProgram>> m_shaderPrograms;

		RendererTable m_rendererTable;
		RenderTaskTable m_renderTaskTable;
	};
}