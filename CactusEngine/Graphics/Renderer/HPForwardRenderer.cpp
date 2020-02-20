#include "HPForwardRenderer.h"
#include "DrawingSystem.h"
#include "AllComponents.h"
#include "Timer.h"
#include "ImageTexture.h"

// For line drawing computation
#include <Eigen/Dense>

using namespace Engine;

HPForwardRenderer::HPForwardRenderer(const std::shared_ptr<DrawingDevice> pDevice, DrawingSystem* pSystem)
	: BaseRenderer(ERendererType::Forward, pDevice, pSystem)
{
}

void HPForwardRenderer::BuildRenderGraph()
{
	m_pRenderGraph = std::make_shared<RenderGraph>();


}

void HPForwardRenderer::Draw(const std::vector<std::shared_ptr<IEntity>>& drawList, const std::shared_ptr<IEntity> pCamera)
{
	if (!pCamera)
	{
		return;
	}

	auto pContext = std::make_shared<RenderContext>();
	pContext->pCamera = pCamera;
	pContext->pDrawList = &drawList;
	pContext->pRenderer = this;

	m_pRenderGraph->BeginRenderPasses(pContext);
}

void HPForwardRenderer::BuildRenderResources()
{
	uint32_t screenWidth  = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowWidth();
	uint32_t screenHeight = gpGlobal->GetConfiguration<GraphicsConfiguration>(EConfigurationType::Graphics)->GetWindowHeight();


}