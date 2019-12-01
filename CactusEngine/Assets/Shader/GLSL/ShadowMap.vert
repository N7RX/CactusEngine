#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

out vec2 v2fTexCoord;

uniform mat4 ModelMatrix;
uniform mat4 LightSpaceMatrix;


void main(void)
{
	v2fTexCoord = inTexCoord;
	gl_Position = LightSpaceMatrix * ModelMatrix * vec4(inPosition, 1.0);
}