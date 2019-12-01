#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

out vec2 v2fTexCoord;
out vec3 v2fNormal;
out vec3 v2fPosition;
out vec3 v2fLightSpacePosition;
out mat3 v2fTBNMatrix;
out vec3 v2fTangent;
out vec3 v2fBitangent;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat3 NormalMatrix;
uniform mat4 LightSpaceMatrix;


void main(void)
{
	v2fTexCoord = inTexCoord;
	v2fNormal = normalize(NormalMatrix * inNormal);
	v2fPosition = (ModelMatrix * vec4(inPosition, 1.0)).xyz;
	v2fLightSpacePosition = (LightSpaceMatrix * vec4(v2fPosition, 1.0)).xyz;
	v2fTBNMatrix = mat3(normalize(NormalMatrix * inTangent), normalize(NormalMatrix * inBitangent), v2fNormal);
	v2fTangent  = inTangent;
	v2fBitangent = inBitangent;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(inPosition, 1.0);
}