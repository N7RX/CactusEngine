#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(std140, binding = 14) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
};

layout(std140, binding = 22) uniform CameraMatrices
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
};


void main(void)
{
	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}