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

	class BaseRenderer
	{
	public:
		BaseRenderer(ERendererType type, GraphicsDevice* pDevice, RenderingSystem* pSystem);
		virtual ~BaseRenderer() = default;

		virtual void Initialize() {}
		virtual void ShutDown() {}

		virtual void BuildRenderGraph() = 0;

		void Draw(const std::vector<BaseEntity*>& drawList, BaseEntity* pCamera, uint32_t frameIndex);
		void WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer);

		ERendererType GetRendererType() const;

		void SetRendererPriority(uint32_t priority);
		uint32_t GetRendererPriority() const;	

		GraphicsDevice* GetGraphicsDevice() const;
		RenderingSystem* GetRenderingSystem() const;

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

		std::unordered_map<uint32_t, GraphicsCommandBuffer*> m_commandRecordReadyList; // Submit Priority - Recorded Command Buffer
		std::unordered_map<uint32_t, bool> m_commandRecordReadyListFlag; // Submit Priority - Ready to submit or has been submitted

		std::mutex m_commandRecordListWriteMutex;
		std::condition_variable m_commandRecordListCv;
		std::queue<uint32_t> m_writtenCommandPriorities;
		bool m_newCommandRecorded;
	};
}