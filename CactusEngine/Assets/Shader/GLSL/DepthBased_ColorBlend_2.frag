#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1;
uniform sampler2D DepthTexture_1;
uniform sampler2D ColorTexture_2;
uniform sampler2D DepthTexture_2;


void main(void)
{
	float alpha = step(0, texture2D(DepthTexture_1, v2fTexCoord).r - texture2D(DepthTexture_2, v2fTexCoord).r);
	outColor = texture2D(ColorTexture_2, v2fTexCoord) * alpha + (1.0f - alpha) * texture2D(ColorTexture_1, v2fTexCoord);
}