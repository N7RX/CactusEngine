#pragma once
#include "BaseComponent.h"
#include "BasicMathTypes.h"

namespace Engine
{
	class TransformComponent : public BaseComponent
	{
	public:
		TransformComponent();
		TransformComponent(Vector3 position, Vector3 scale, Vector3 rotation);
		~TransformComponent() = default;

		Vector3 GetPosition() const;
		Vector3 GetScale() const;
		Vector3 GetRotation() const;

		void SetPosition(Vector3 newPosition);
		void SetScale(Vector3 newScale);
		void SetRotation(Vector3 newRotation);

		Vector3 GetForwardDirection() const;
		Vector3 GetRightDirection() const;

		Matrix4x4 GetModelMatrix() const;

	private:
		Vector3 m_position;
		Vector3 m_scale;
		Vector3 m_rotationEuler;
		Quaternion m_rotationQuaternion; // TODO: implement quaternion compuation

		Vector3 m_forwardDirection;
		Vector3 m_rightDirection;
	};
}