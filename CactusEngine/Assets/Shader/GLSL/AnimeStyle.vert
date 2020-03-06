#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 v2fTexCoord;
layout(location = 1) out vec3 v2fNormal;
layout(location = 2) out vec3 v2fPosition;
layout(location = 3) out vec3 v2fLightSpacePosition;
layout(location = 4) out vec3 v2fTangent;
layout(location = 5) out vec3 v2fBitangent;
layout(location = 6) out mat3 v2fTBNMatrix;

layout(std140, binding = 14) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 NormalMatrix;
};

layout(std140, binding = 15) uniform LightSpaceTransformMatrix
{
	mat4 LightSpaceMatrix;
};


void main(void)
{
	v2fTexCoord = inTexCoord;
	v2fNormal = normalize(mat3(NormalMatrix) * inNormal);
	v2fPosition = (ModelMatrix * vec4(inPosition, 1.0)).xyz;
	v2fLightSpacePosition = (LightSpaceMatrix * vec4(v2fPosition, 1.0)).xyz;
	v2fTangent  = inTangent;
	v2fBitangent = inBitangent;
	v2fTBNMatrix = mat3(normalize(mat3(NormalMatrix) * inTangent), normalize(mat3(NormalMatrix) * inBitangent), v2fNormal);

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}