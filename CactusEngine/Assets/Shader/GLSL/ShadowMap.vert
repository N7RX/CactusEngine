#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 v2fTexCoord;

layout(std140, binding = 14) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
};

layout(std140, binding = 15) uniform LightSpaceTransformMatrix
{
	mat4 LightSpaceMatrix;
};


void main(void)
{
	v2fTexCoord = inTexCoord;
	gl_Position = LightSpaceMatrix * ModelMatrix * vec4(inPosition, 1.0);
}