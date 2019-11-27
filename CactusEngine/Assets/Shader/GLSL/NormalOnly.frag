#version 430

in vec3 v2fNormal;

layout(location = 0) out vec4 outColor;


void main(void)
{
	outColor = vec4(v2fNormal, 1.0f);
}