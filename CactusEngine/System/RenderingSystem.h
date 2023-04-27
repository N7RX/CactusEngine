#pragma once
#include "BaseRenderer.h"
#include "ECSWorld.h"
#include "BuiltInShaderType.h"
#include "SafeQueue.h"

namespace Engine
{
	class ShaderProgram;
	class BaseWindow;

	class RenderingSystem : public BaseSystem
	{
	public:
		RenderingSystem(ECSWorld* pWorld);
		~RenderingSystem() = default;

		void Initialize() override;
		void ShutDown() override;

		void FrameBegin() override;
		void Tick() override;
		void FrameEnd() override;

		void WaitUntilFinish() override;

		EGraphicsAPIType GetGraphicsAPIType() const;
		ShaderProgram* GetShaderProgramByType(EBuiltInShaderProgramType type);

		template<typename T>
		inline void RegisterRenderer(ERendererType type, uint32_t priority = 0)
		{
			T* pRenderer;
			CE_NEW(pRenderer, T, m_pDevice, this); // TODO: replace this temp solution to reference current System
			pRenderer->SetRendererPriority(priority);
			m_rendererTable[(uint32_t)type] = pRenderer;
		}

		void RemoveRenderer(ERendererType type);
		void SetActiveRenderer(ERendererType type);

		void SetDeviceWindow(BaseWindow* pWindow);

		// For OpenGL only; takes context ownership from render thread and may cause a stall
		void AcquireRenderContextOwnership();
		void ReleaseRenderContextOwnership();

		void UpdateResolution();

	private:
		bool CreateDevice();

		void RegisterRenderers();
		void InitializeActiveRenderer();

		void LoadAllShaders();
		bool LoadShader(EBuiltInShaderProgramType type);

		void RenderThreadFunction();
		void BuildRenderTask();
		void ExecuteRenderTask();

		void UpdateResolutionImpl();

	private:
		ECSWorld* m_pECSWorld;
		GraphicsDevice* m_pDevice;
		bool m_isRunning;

		std::vector<ShaderProgram*> m_shaderPrograms;
		std::mutex m_shaderProgramsMutex;

		ERendererType m_activeRenderer;
		BaseRenderer* m_rendererTable[(uint32_t)ERendererType::COUNT];

		enum class ERenderPhaseType
		{
			Invalid = 0,
			Tick,
			FrameEnd
		};
		std::thread m_renderThread;
		ThreadSemaphore m_renderFinishSemaphore;
		std::mutex m_renderThreadMutex;
		std::condition_variable m_renderThreadCv;
		SafeQueue<ERenderPhaseType> m_renderPhaseQueue;

		std::vector<BaseEntity*> m_opaqueDrawList;
		std::vector<BaseEntity*> m_transparentDrawList;
		std::vector<BaseEntity*> m_lightDrawList;
		
		uint32_t m_frameIndex;
		uint32_t m_maxFramesInFlight;

		bool m_pendingResolutionUpdate;
		bool m_pauseRendering;

		BaseWindow* m_pCurrentWindow;
	};
}