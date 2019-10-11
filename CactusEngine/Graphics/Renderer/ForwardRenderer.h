#pragma once
#include "BaseRenderer.h"

namespace Engine
{
	class ForwardRenderer : public BaseRenderer, std::enable_shared_from_this<ForwardRenderer>
	{
	public:
		ForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~ForwardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;

	private:
		void BuildOpaquePass();
		void BuildTransparentPass();

	private:
		std::shared_ptr<FrameBuffer> m_pOpaquePassFrameOutput;
	};
}