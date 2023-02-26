#include "MeshRendererComponent.h"

namespace Engine
{
	MeshRendererComponent::MeshRendererComponent()
		: BaseComponent(EComponentType::MeshRenderer), m_rendererType(ERendererType::Standard)
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
}