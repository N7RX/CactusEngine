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

		void SetMesh(Mesh* pMesh);
		Mesh* GetMesh() const;

	private:
		Mesh* m_pMesh;
	};
}