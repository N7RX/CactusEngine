#pragma once
#include "BaseComponent.h"
#include "Mesh.h"

namespace Engine
{
	class MeshFilterComponent : public BaseComponent
	{
	public:
		MeshFilterComponent();
		~MeshFilterComponent() = default;

		void SetMesh(const std::shared_ptr<Mesh> pMesh);
		std::shared_ptr<Mesh> GetMesh() const;

	private:
		std::shared_ptr<Mesh> m_pMesh;
	};
}