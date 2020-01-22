#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

out vec3 v2fNormal;
out vec3 v2fPosition;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat3 NormalMatrix;


void main(void)
{
	v2fNormal = normalize(NormalMatrix * inNormal);
	v2fPosition = (ModelMatrix * vec4(inPosition, 1.0)).xyz;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}