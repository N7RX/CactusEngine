#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 v2fNormal;
layout(location = 1) out vec3 v2fPosition;

layout(std140, binding = 0) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 NormalMatrix;
};


void main(void)
{
	v2fNormal = normalize(mat3(NormalMatrix) * inNormal);
	v2fPosition = (ModelMatrix * vec4(inPosition, 1.0)).xyz;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}