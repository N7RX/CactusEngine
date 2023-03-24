#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 v2fTexCoord;
layout(location = 1) out vec3 v2fNormal;
layout(location = 2) out vec3 v2fPosition;

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

layout(std140, binding = 18) uniform SystemVariables
{
	float Time;
};

layout(binding = 9) uniform sampler2D NoiseTexture_1;


void main(void)
{
	v2fTexCoord = inTexCoord;
	v2fNormal = normalize(mat3(NormalMatrix) * inNormal);
	v2fPosition = (ModelMatrix * vec4(inPosition, 1.0)).xyz;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}