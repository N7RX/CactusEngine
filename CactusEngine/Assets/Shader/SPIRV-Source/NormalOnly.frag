#version 430

layout(location = 0) in vec3 v2fNormal;
layout(location = 1) in vec3 v2fPosition;

layout(location = 0) out vec4 outColorNormal;
layout(location = 1) out vec4 outColorPosition;


void main(void)
{
	outColorNormal = vec4(v2fNormal, 1.0f);
	outColorPosition = vec4(v2fPosition, 1.0f);
}