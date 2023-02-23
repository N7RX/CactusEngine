#include "BaseRenderer.h"

using namespace Engine;

BaseRenderer::BaseRenderer(ERendererType type, const std::shared_ptr<GraphicsDevice> pDevice, RenderingSystem* pSystem)
	: m_rendererType(type), m_renderPriority(0), m_pDevice(pDevice), m_pSystem(pSystem)
{
	m_eGraphicsDeviceType = m_pDevice->GetGraphicsAPIType();
}

ERendererType BaseRenderer::GetRendererType() const
{
	return m_rendererType;
}

void BaseRenderer::SetRendererPriority(uint32_t priority)
{
	m_renderPriority = priority;
}

uint32_t BaseRenderer::GetRendererPriority() const
{
	return m_renderPriority;
}

std::shared_ptr<GraphicsDevice> BaseRenderer::GetGraphicsDevice() const
{
	return m_pDevice;
}

RenderingSystem* BaseRenderer::GetRenderingSystem() const
{
	return m_pSystem;
}