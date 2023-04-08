#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 6) uniform sampler2D ColorTexture_1;
layout(binding = 4) uniform sampler2D DepthTexture_1;
layout(binding = 7) uniform sampler2D ColorTexture_2;
layout(binding = 5) uniform sampler2D DepthTexture_2;

const float CameraGamma = 2.2f;


void main(void)
{
	float depthFromOpaque = texture(DepthTexture_1, v2fTexCoord).r;
	float depthFromTransp = texture(DepthTexture_2, v2fTexCoord).r;

	float alpha = step(0, depthFromOpaque - depthFromTransp);

	vec4 colorFromOpaque = texture(ColorTexture_1, v2fTexCoord);
	vec4 colorFromTransp = texture(ColorTexture_2, v2fTexCoord);

	outColor = colorFromTransp * alpha + (1.0f - alpha) * colorFromOpaque;

	// Gamma correction
	outColor.xyz = pow(outColor.xyz, vec3(1.0 / CameraGamma));
}