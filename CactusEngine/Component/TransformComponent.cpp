#include "TransformComponent.h"

using namespace Engine;

TransformComponent::TransformComponent()
	: BaseComponent(eCompType_Transform), m_position(Vector3(0, 0, 0)), m_scale(Vector3(1, 1, 1)), m_rotationEuler(Vector3(0, 0, 0)), 
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0))
{
}

TransformComponent::TransformComponent(Vector3 position, Vector3 scale, Vector3 rotation)
	: BaseComponent(eCompType_Transform), m_position(position), m_scale(scale), m_rotationEuler(rotation), 
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0))
{
}

Vector3 TransformComponent::GetPosition() const
{
	return m_position;
}

Vector3 TransformComponent::GetScale() const
{
	return m_scale;
}

Vector3 TransformComponent::GetRotation() const
{
	return m_rotationEuler;
}

void TransformComponent::SetPosition(Vector3 newPosition)
{
	m_position = newPosition;
}

void TransformComponent::SetScale(Vector3 newScale)
{
	m_scale = newScale;
}

void TransformComponent::SetRotation(Vector3 newRotation)
{
	m_rotationEuler = newRotation;
}

Vector3 TransformComponent::GetForwardDirection() const
{
	return m_forwardDirection;
}

Vector3 TransformComponent::GetRightDirection() const
{
	return m_rightDirection;
}

Matrix4x4 TransformComponent::GetModelMatrix() const
{
	Matrix4x4 modelMat =
		glm::translate(m_position)
		* (glm::rotate(m_rotationEuler.x * D2R, X_AXIS)*glm::rotate(m_rotationEuler.y * D2R, Y_AXIS)*glm::rotate(m_rotationEuler.z * D2R, Z_AXIS))
		* glm::scale(m_scale);

	return modelMat;
}