#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 6) uniform sampler2D ColorTexture_1; // Main alpha source
layout(binding = 7) uniform sampler2D ColorTexture_2;

const vec4 LineColor = vec4(0.0f);


void main(void)
{
	float alpha = texture(ColorTexture_1, v2fTexCoord).a;
	outColor = LineColor * alpha + (1.0f - alpha) * texture(ColorTexture_2, v2fTexCoord);
}