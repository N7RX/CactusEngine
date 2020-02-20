#pragma once
#include "BaseRenderer.h"
#include "RenderTexture.h"

namespace Engine
{
	// High-performance forward renderer; dedicated for Vulkan device
	class HPForwardRenderer : public BaseRenderer, std::enable_shared_from_this<HPForwardRenderer>
	{
	public:
		HPForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem);
		~HPForwardRenderer() = default;

		void BuildRenderGraph() override;
		void Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera) override;

	private:
		void BuildRenderResources();

		void BuildShadowMapPass();
		void BuildNormalOnlyPass();
		void BuildOpaquePass();
		void BuildGaussianBlurPass();
		void BuildLineDrawingPass();
		void BuildTransparentPass();
		void BuildOpaqueTranspBlendPass();
		void BuildDepthOfFieldPass();

		void CreateLineDrawingMatrices();

	private:

	};

	namespace HPForwardGraphRes
	{

	}
}