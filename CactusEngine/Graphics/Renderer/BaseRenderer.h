#pragma once
#include "IRenderer.h"
#include "IEntity.h"
#include "DrawingDevice.h"

namespace Engine
{
	class DrawingSystem;
	class BaseRenderer : public IRenderer
	{
	public:
		BaseRenderer(ERendererType type, const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);

		virtual void Initialize() {}
		virtual void ShutDown() {}

		virtual void FrameBegin() {}
		virtual void Tick() {}
		virtual void FrameEnd() {}

		virtual void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) = 0;

		ERendererType GetRendererType() const;

		void SetRendererPriority(uint32_t priority);
		uint32_t GetRendererPriority() const;

	protected:
		ERendererType m_rendererType;
		uint32_t m_renderPriority;
		std::shared_ptr<DrawingDevice> m_pDevice;
		DrawingSystem* m_pSystem;
	};
}