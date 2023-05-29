#pragma once
#include "SharedTypes.h"
#include "SafeBasicTypes.h"

#include <unordered_map>
#include <queue>
#include <vector>

namespace Engine
{
	class BaseEntity;
	class RenderingSystem;
	class RenderGraph;
	class GraphicsDevice;
	class GraphicsCommandBuffer;
	class RenderGraphResource;
	class Texture2D;
	class UniformBufferManager;

	struct RenderContext;

	class BaseRenderer
	{
	public:
		BaseRenderer(ERendererType type, GraphicsDevice* pDevice, RenderingSystem* pSystem);
		virtual ~BaseRenderer() = default;

		virtual void Initialize() {}
		virtual void ShutDown() {}

		virtual void BuildRenderGraph() = 0;

		void Draw(const RenderContext& renderContext, uint32_t frameIndex);
		void WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer);

		ERendererType GetRendererType() const;

		void SetRendererPriority(uint32_t priority);
		uint32_t GetRendererPriority() const;	

		RenderGraph* GetRenderGraph() const;
		GraphicsDevice* GetGraphicsDevice() const;
		RenderingSystem* GetRenderingSystem() const;
		UniformBufferManager* GetBufferManager() const;

		void UpdateResolution(uint32_t width, uint32_t height);

	protected:
		void ObtainSwapchainImages();

	protected:
		ERendererType	 m_rendererType;
		uint32_t		 m_renderPriority; // 0 is the highest priority
		RenderGraph*	 m_pRenderGraph;
		GraphicsDevice*	 m_pDevice;
		RenderingSystem* m_pSystem;
		EGraphicsAPIType m_eGraphicsDeviceType;

		std::vector<RenderGraphResource*> m_graphResources;
		std::vector<Texture2D*> m_swapchainImages;

		std::vector<GraphicsCommandBuffer*> m_commandRecordReadyList;

		std::mutex m_commandRecordListWriteMutex;
		std::condition_variable m_commandRecordListCv;
		uint32_t m_finishedNodeCount;
		bool m_commandRecordFinished;

		UniformBufferManager* m_pBufferManager;
	};
}