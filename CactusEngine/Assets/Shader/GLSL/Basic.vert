#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

out vec2 v2fTexCoord;
out vec3 v2fNormal;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat3 NormalMatrix;


void main(void)
{
	v2fTexCoord = inTexCoord;
	v2fNormal = NormalMatrix * inNormal;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}