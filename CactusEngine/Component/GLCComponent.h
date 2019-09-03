#pragma once
#include "BaseComponent.h"
#include <vector>

namespace Engine
{
	class GLCComponent;
	class GLCRay
	{
	public:
		GLCRay(GLCComponent* pParentGLC);
		GLCRay(GLCComponent* pParentGLC, Vector2 origin, Vector2 direction);
		~GLCRay() = default;

		void SetOrigin(Vector2 origin);
		void SetDirection(Vector2 direction);
		Vector2 GetOrigin() const;
		Vector2 GetDirection() const;

		Vector4 GetRayDefinition() const;
		Vector3 GetPointOnRay(float t) const;

	private:
		GLCComponent* m_pParentGLC;
		Vector2 m_origin, m_direction;
	};

	class GLCComponent : public BaseComponent
	{
	public:
		GLCComponent();
		~GLCComponent() = default;

		float GetFirstImagePlane();
		float GetSecondImagePlane();

		void SetSecondImagePlane(float val);

		std::shared_ptr<GLCRay> GetRayGen(unsigned int index) const;

	private:
		float m_firstImagePlane;
		float m_secondImagePlane;
		std::vector<std::shared_ptr<GLCRay>> m_pRayGen;
	};
}