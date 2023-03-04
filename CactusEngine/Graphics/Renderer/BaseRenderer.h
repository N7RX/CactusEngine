#pragma once
#include "SharedTypes.h"

#include <vector>

namespace Engine
{
	class BaseEntity;
	class RenderingSystem;
	class RenderGraph;
	class GraphicsDevice;
	class GraphicsCommandBuffer;

	class BaseRenderer
	{
	public:
		BaseRenderer(ERendererType type, GraphicsDevice* pDevice, RenderingSystem* pSystem);
		virtual ~BaseRenderer() = default;

		virtual void Initialize() {}
		virtual void ShutDown() {}

		virtual void FrameBegin() {}
		virtual void Tick() {}
		virtual void FrameEnd() {}

		virtual void BuildRenderGraph() = 0;
		virtual void Draw(const std::vector<BaseEntity*>& drawList, BaseEntity* pCamera) = 0;
		virtual void WriteCommandRecordList(const char* pNodeName, GraphicsCommandBuffer* pCommandBuffer) = 0;

		ERendererType GetRendererType() const;

		void SetRendererPriority(uint32_t priority);
		uint32_t GetRendererPriority() const;

		GraphicsDevice* GetGraphicsDevice() const;
		RenderingSystem* GetRenderingSystem() const;

	protected:
		ERendererType m_rendererType;
		uint32_t m_renderPriority;
		RenderGraph* m_pRenderGraph;
		GraphicsDevice* m_pDevice;
		RenderingSystem* m_pSystem;
		EGraphicsAPIType m_eGraphicsDeviceType;
	};
}