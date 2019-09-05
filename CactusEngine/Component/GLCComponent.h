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
		GLCRay(GLCComponent* pParentGLC, Vector2 origin, Vector2 imagePlanePoint);
		~GLCRay() = default;

		void SetOrigin(Vector2 origin);
		void SetImagePlanePoint(Vector2 imagePlanePoint);
		void SetByDefinition(Vector4 definition);

		Vector3 GetOrigin() const;
		Vector3 GetDirection() const;

		Vector4 GetDefinition() const;
		Vector3 GetPointOnRay(float t) const;

	private:
		GLCComponent* m_pParentGLC;
		Vector3 m_origin, m_imagePlanePoint, m_direction;
	};

	class GLCComponent : public BaseComponent
	{
	public:
		GLCComponent();
		~GLCComponent() = default;

		float GetFirstImagePlane();
		float GetSecondImagePlane();

		void SetSecondImagePlane(float val);

		std::shared_ptr<GLCRay> GetRayDefinition(unsigned int index) const;
		std::shared_ptr<GLCRay> GenerateRay(unsigned int columnIndex, unsigned int rowIndex) const;

		void UpdateParameters();

	public:
		static float m_sSecondImagePlane;

	private:
		float m_firstImagePlane;
		std::vector<std::shared_ptr<GLCRay>> m_pRayDefinition;
		float m_coeffA, m_coeffB, m_coeffC;
	};
}