#pragma once
#include "BaseComponent.h"
#include "IRenderer.h"
#include <memory>

namespace Engine
{
	class MeshRendererComponent : public BaseComponent
	{
	public:
		MeshRendererComponent();
		~MeshRendererComponent() = default;

		void SetRenderer(ERendererType rendererType);
		ERendererType GetRendererType() const;

	private:
		ERendererType m_rendererType; // TODO: deprecate this property
		// TODO: add additional render properties here
	};
}