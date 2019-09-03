#include "MeshRendererComponent.h"

using namespace Engine;

MeshRendererComponent::MeshRendererComponent()
	: BaseComponent(eCompType_MeshRenderer)
{
}

void MeshRendererComponent::SetRenderer(ERendererType rendererType)
{
	m_rendererType = rendererType;
}

ERendererType MeshRendererComponent::GetRendererType() const
{
	return m_rendererType;
}