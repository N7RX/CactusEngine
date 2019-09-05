#include "GLCComponent.h"
#include <assert.h>

using namespace Engine;

GLCRay::GLCRay(GLCComponent* pParentGLC)
	: m_pParentGLC(pParentGLC), m_origin(0, 0, 0), m_imagePlanePoint(0, 0, 0), m_direction(0, 0, 0)
{
}

GLCRay::GLCRay(GLCComponent* pParentGLC, Vector2 origin, Vector2 imagePlanePoint)
	: m_pParentGLC(pParentGLC), m_origin(origin, pParentGLC->GetFirstImagePlane()), m_imagePlanePoint(imagePlanePoint, pParentGLC->GetSecondImagePlane())
{
	m_direction = m_imagePlanePoint - m_origin;
}

void GLCRay::SetOrigin(Vector2 origin)
{
	m_origin = Vector3(origin, m_pParentGLC->GetFirstImagePlane());
	m_direction = m_imagePlanePoint - m_origin;
}

void GLCRay::SetImagePlanePoint(Vector2 imagePlanePoint)
{
	m_imagePlanePoint = Vector3(imagePlanePoint, m_pParentGLC->GetSecondImagePlane());
	m_direction = m_imagePlanePoint - m_origin;
}

void GLCRay::SetByDefinition(Vector4 definition)
{
	m_origin = Vector3(definition.z, definition.w, m_pParentGLC->GetFirstImagePlane());
	m_direction = Vector3(definition.x, definition.y, m_pParentGLC->GetSecondImagePlane() - m_pParentGLC->GetFirstImagePlane());
	m_imagePlanePoint = Vector3(definition.x + definition.z, definition.y + definition.w, m_pParentGLC->GetSecondImagePlane());
}

Vector3 GLCRay::GetOrigin() const
{
	return m_origin;
}

Vector3 GLCRay::GetDirection() const
{
	return m_direction;
}

Vector4 GLCRay::GetDefinition() const
{
	return Vector4(m_direction.x, m_direction.y, m_origin.x, m_origin.y);
}

Vector3 GLCRay::GetPointOnRay(float t) const
{
	return m_origin + t * m_direction;
}

float GLCComponent::m_sSecondImagePlane = 1.0f;

GLCComponent::GLCComponent()
	: BaseComponent(eCompType_GLC), m_firstImagePlane(0)
{
	m_pRayDefinition.resize(3);
	for (int i = 0; i < 3; ++i)
	{
		m_pRayDefinition[i] = std::make_shared<GLCRay>(this);
	}
}

void GLCComponent::SetSecondImagePlane(float val)
{
	m_sSecondImagePlane = val;
}

float GLCComponent::GetFirstImagePlane()
{
	return m_firstImagePlane;
}

float GLCComponent::GetSecondImagePlane()
{
	return m_sSecondImagePlane;
}

std::shared_ptr<GLCRay> GLCComponent::GetRayDefinition(unsigned int index) const
{
	assert(index < m_pRayDefinition.size());
	return m_pRayDefinition[index];
}

std::shared_ptr<GLCRay> GLCComponent::GenerateRay(unsigned int columnIndex, unsigned int rowIndex) const
{
	Matrix3x3 alphaMat;
	alphaMat[0][0] = m_sSecondImagePlane * m_pRayDefinition[0]->GetDirection()[0]; alphaMat[1][0] = m_sSecondImagePlane * m_pRayDefinition[0]->GetDirection()[1];		   alphaMat[2][0] = 1.0f;
	alphaMat[0][1] = float(columnIndex);										   alphaMat[1][1] = float(rowIndex);													   alphaMat[2][1] = 1.0f;
	alphaMat[0][2] = m_sSecondImagePlane * m_pRayDefinition[2]->GetDirection()[0]; alphaMat[1][2] = 1.0f + m_sSecondImagePlane * m_pRayDefinition[2]->GetDirection()[1];   alphaMat[2][2] = 1.0f;
	float alpha = glm::determinant(alphaMat) / (m_coeffA*m_sSecondImagePlane*m_sSecondImagePlane + m_coeffB * m_sSecondImagePlane + m_coeffC);

	Matrix3x3 betaMat;
	betaMat[0][0] = m_sSecondImagePlane * m_pRayDefinition[0]->GetDirection()[0];		 betaMat[1][0] = m_sSecondImagePlane * m_pRayDefinition[0]->GetDirection()[1]; betaMat[2][0] = 1.0f;
	betaMat[0][1] = 1.0f + m_sSecondImagePlane * m_pRayDefinition[1]->GetDirection()[0]; betaMat[1][1] = m_sSecondImagePlane * m_pRayDefinition[1]->GetDirection()[1]; betaMat[2][1] = 1.0f;
	betaMat[0][2] = float(columnIndex);													 betaMat[1][2] = float(rowIndex);											   betaMat[2][2] = 1.0f;
	float beta = glm::determinant(betaMat) / (m_coeffA*m_sSecondImagePlane*m_sSecondImagePlane + m_coeffB * m_sSecondImagePlane + m_coeffC);

	Vector4 generatedDefinition = alpha * m_pRayDefinition[1]->GetDefinition() + beta * m_pRayDefinition[2]->GetDefinition() + (1.0f - alpha - beta) * m_pRayDefinition[0]->GetDefinition();

	auto pRay = std::make_shared<GLCRay>((GLCComponent*)this);
	pRay->SetByDefinition(generatedDefinition);

	return pRay;
}

void GLCComponent::UpdateParameters()
{
	Matrix3x3 A;
	for (unsigned int row = 0; row < 3; ++row)
	{
		A[0][row] = m_pRayDefinition[row]->GetDirection()[0];
		A[1][row] = m_pRayDefinition[row]->GetDirection()[1];
		A[2][row] = 1.0f;
	}
	m_coeffA = glm::determinant(A);

	Matrix3x3 B1;
	for (unsigned int row = 0; row < 3; ++row)
	{
		B1[0][row] = m_pRayDefinition[row]->GetDirection()[0];
		B1[1][row] = m_pRayDefinition[row]->GetOrigin()[1];
		B1[2][row] = 1.0f;
	}
	Matrix3x3 B2;
	for (unsigned int row = 0; row < 3; ++row)
	{
		B2[0][row] = m_pRayDefinition[row]->GetDirection()[1];
		B2[1][row] = m_pRayDefinition[row]->GetOrigin()[0];
		B2[2][row] = 1.0f;
	}
	m_coeffB = glm::determinant(B1) - glm::determinant(B2);

	Matrix3x3 C;
	for (unsigned int row = 0; row < 3; ++row)
	{
		C[0][row] = m_pRayDefinition[row]->GetOrigin()[0];
		C[1][row] = m_pRayDefinition[row]->GetOrigin()[1];
		C[2][row] = 1.0f;
	}
	m_coeffC = glm::determinant(C);
}