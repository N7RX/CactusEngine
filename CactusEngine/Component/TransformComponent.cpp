#include "TransformComponent.h"

using namespace Engine;

#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
uint32_t TransformComponent::m_sharedBufferAssignID = 0;
Vector4 TransformComponent::Shared_PositionBuffer_Async[513] = { Vector4(0) };
#endif

TransformComponent::TransformComponent()
	: BaseComponent(EComponentType::Transform), m_position(Vector3(0)), m_scale(Vector3(1)), m_rotationEuler(Vector3(0, -90, 0)),
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0)), m_rotationQuaternion(Vector4(0))
{
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	m_assginedBufferID = m_sharedBufferAssignID;
	m_sharedBufferAssignID++;
#endif
}

TransformComponent::TransformComponent(Vector3 position, Vector3 scale, Vector3 rotation)
	: BaseComponent(EComponentType::Transform), m_position(position), m_scale(scale), m_rotationEuler(rotation),
	m_forwardDirection(Vector3(0, 0, -1)), m_rightDirection(Vector3(1, 0, 0)), m_rotationQuaternion(Vector4(0))
{
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	m_assginedBufferID = m_sharedBufferAssignID;
	m_sharedBufferAssignID++;
#endif
}

Vector3 TransformComponent::GetPosition() const
{
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	Vector3 overridePosition = Vector3(Shared_PositionBuffer_Async[m_assginedBufferID].x, Shared_PositionBuffer_Async[m_assginedBufferID].y, Shared_PositionBuffer_Async[m_assginedBufferID].z);
	return overridePosition;
#endif
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
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	Shared_PositionBuffer_Async[m_assginedBufferID] = Vector4(newPosition, 0);
	return;
#endif
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
#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	Matrix4x4 modelMat =
		glm::translate(Vector3(Shared_PositionBuffer_Async[m_assginedBufferID].x, Shared_PositionBuffer_Async[m_assginedBufferID].y, Shared_PositionBuffer_Async[m_assginedBufferID].z))
#else
	Matrix4x4 modelMat =
		glm::translate(m_position)
#endif
		* (glm::rotate(m_rotationEuler.x * D2R, X_AXIS)*glm::rotate(m_rotationEuler.y * D2R, Y_AXIS)*glm::rotate(m_rotationEuler.z * D2R, Z_AXIS))
		* glm::scale(m_scale);

	return modelMat;
}

Matrix4x4 TransformComponent::GetNormalMatrix() const
{
	Matrix4x4 normalMat =
		glm::rotate(m_rotationEuler.x * D2R, X_AXIS)*glm::rotate(m_rotationEuler.y * D2R, Y_AXIS)*glm::rotate(m_rotationEuler.z * D2R, Z_AXIS)
		* glm::inverse(glm::scale(m_scale));

	return normalMat;
}