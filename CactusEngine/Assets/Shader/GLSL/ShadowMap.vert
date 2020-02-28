#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 v2fTexCoord;

layout(std140, binding = 0) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 NormalMatrix;
};

layout(std140, binding = 1) uniform LightSpaceTransformMatrix
{
	mat4 LightSpaceMatrix;
};


void main(void)
{
	v2fTexCoord = inTexCoord;
	gl_Position = LightSpaceMatrix * ModelMatrix * vec4(inPosition, 1.0);
}