#include "TransformComponent.h"

using namespace Engine;

TransformComponent::TransformComponent()
	: BaseComponent(EComponentType::Transform), m_position(Vector3(0)), m_scale(Vector3(1)), m_rotationEuler(Vector3(0, -90, 0)),
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0)), m_rotationQuaternion(Vector4(0))
{
}

TransformComponent::TransformComponent(Vector3 position, Vector3 scale, Vector3 rotation)
	: BaseComponent(EComponentType::Transform), m_position(position), m_scale(scale), m_rotationEuler(rotation),
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0)), m_rotationQuaternion(Vector4(0))
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
	//Vector3 diff = newRotation - m_rotationEuler;
	m_rotationEuler = newRotation;

	// Alert: this could be buggy since z angle is not taken into account
	m_forwardDirection.x = cos(m_rotationEuler.y * D2R) * cos(m_rotationEuler.x * D2R);
	m_forwardDirection.y = sin(m_rotationEuler.x * D2R);
	m_forwardDirection.z = sin(m_rotationEuler.y * D2R) * cos(m_rotationEuler.x * D2R);

	m_forwardDirection = glm::normalize(m_forwardDirection);

	//m_forwardDirection = Vector3(
	//	(glm::rotate(diff.x * D2R, X_AXIS)*glm::rotate(diff.y * D2R, Y_AXIS)*glm::rotate(diff.z * D2R, Z_AXIS))
	//	* Vector4(m_forwardDirection, 1.0f));

	m_rightDirection = glm::normalize(glm::cross(m_forwardDirection, UP));
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

Matrix3x3 TransformComponent::GetNormalMatrix() const
{
	Matrix3x3 normalMat =
		Matrix3x3(glm::rotate(m_rotationEuler.x * D2R, X_AXIS)*glm::rotate(m_rotationEuler.y * D2R, Y_AXIS)*glm::rotate(m_rotationEuler.z * D2R, Z_AXIS))
		* glm::inverse(Matrix3x3(glm::scale(m_scale)));

	return normalMat;
}