#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{
	typedef glm::vec2 Vector2;
	typedef glm::vec3 Vector3;
	typedef glm::vec4 Vector4;

	typedef glm::vec3 Color3;
	typedef glm::vec4 Color4;

	typedef glm::vec4 Quaternion;

	typedef glm::mat2 Matrix2x2;
	typedef glm::mat3 Matrix3x3;
	typedef glm::mat4 Matrix4x4;

	const Vector3 X_AXIS = Vector3(1.0f, 0.0f, 0.0f);
	const Vector3 Z_AXIS = Vector3(0.0f, 0.0f, 1.0f);
	const Vector3 Y_AXIS = Vector3(0.0f, 1.0f, 0.0f);

	const Vector3 UP = Vector3(0.0f, 1.0f, 0.0f);

	const float D2R = 3.1415926536f / 180.0f;
}