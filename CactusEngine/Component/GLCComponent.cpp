#include "GLCComponent.h"
#include <assert.h>

using namespace Engine;

GLCRay::GLCRay(GLCComponent* pParentGLC)
	: m_pParentGLC(pParentGLC)
{
}

GLCRay::GLCRay(GLCComponent* pParentGLC, Vector2 origin, Vector2 direction)
	: m_pParentGLC(pParentGLC), m_origin(origin), m_direction(direction)
{
}

void GLCRay::SetOrigin(Vector2 origin)
{
	m_origin = origin;
}

void GLCRay::SetDirection(Vector2 direction)
{
	m_direction = direction;
}

Vector2 GLCRay::GetOrigin() const
{
	return m_origin;
}

Vector2 GLCRay::GetDirection() const
{
	return m_direction;
}

Vector4 GLCRay::GetRayDefinition() const
{
	return Vector4(m_origin.x, m_origin.y, m_direction.x, m_direction.y);
}

Vector3 GLCRay::GetPointOnRay(float t) const
{
	return Vector3(m_origin.x, m_origin.y, m_pParentGLC->GetFirstImagePlane()) 
		+ t * Vector3(m_direction.x, m_direction.y, m_pParentGLC->GetSecondImagePlane());
}

GLCComponent::GLCComponent()
	: BaseComponent(eCompType_GLC), m_firstImagePlane(0), m_secondImagePlane(1)
{
	m_pRayGen.resize(3);
	for (int i = 0; i < 3; ++i)
	{
		m_pRayGen[i] = std::make_shared<GLCRay>(this);
	}
}

void GLCComponent::SetSecondImagePlane(float val)
{
	m_secondImagePlane = val;
}

float GLCComponent::GetFirstImagePlane()
{
	return m_firstImagePlane;
}

float GLCComponent::GetSecondImagePlane()
{
	return m_secondImagePlane;
}

std::shared_ptr<GLCRay> GLCComponent::GetRayGen(unsigned int index) const
{
	assert(index < m_pRayGen.size());
	return m_pRayGen[index];
}