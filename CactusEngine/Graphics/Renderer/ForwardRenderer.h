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
		void BuildFrameResources();
		void BuildOpaquePass();
		void BuildTransparentPass();
		void BuildOpaqueTranspBlendPass();

	private:
		std::shared_ptr<FrameBuffer> m_pOpaquePassFrameBuffer;
		std::shared_ptr<Texture2D> m_pOpaquePassColorOutput;
		std::shared_ptr<Texture2D> m_pOpaquePassDepthOutput;

		std::shared_ptr<FrameBuffer> m_pTranspPassFrameBuffer;
		std::shared_ptr<Texture2D> m_pTranspPassColorOutput;
		std::shared_ptr<Texture2D> m_pTranspPassDepthOutput;
	};
}