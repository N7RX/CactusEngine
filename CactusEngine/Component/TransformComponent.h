#pragma once
#include "BaseComponent.h"
#include "BasicMathTypes.h"
#include "Global.h"

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
		Matrix4x4 GetNormalMatrix() const;

#if defined(ENABLE_ASYNC_COMPUTE_TEST_CE)
	public:
		static Vector4 Shared_PositionBuffer_Async[513];	

	private:
		static uint32_t m_sharedBufferAssignID;
		uint32_t m_assginedBufferID;
#endif
	private:
		Vector3 m_position;
		Vector3 m_scale;
		Vector3 m_rotationEuler;
		Quaternion m_rotationQuaternion; // TODO: implement quaternion compuation

		Vector3 m_forwardDirection;
		Vector3 m_rightDirection;
	};
}