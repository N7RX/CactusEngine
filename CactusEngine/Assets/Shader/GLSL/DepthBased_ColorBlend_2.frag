#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1;
uniform sampler2D DepthTexture_1;
uniform sampler2D ColorTexture_2;
uniform sampler2D DepthTexture_2;

uniform float CameraGamma = 0.6f;
uniform float Exposure = 2.0f;


void main(void)
{
	float depthFromOpaque = texture2D(DepthTexture_1, v2fTexCoord).r;
	float depthFromTransp = texture2D(DepthTexture_2, v2fTexCoord).r;

	float alpha = step(0, depthFromOpaque - depthFromTransp);

	vec4 colorFromOpaque = texture2D(ColorTexture_1, v2fTexCoord);
	vec4 colorFromTransp = texture2D(ColorTexture_2, v2fTexCoord);

	outColor = colorFromTransp * alpha + (1.0f - alpha) * colorFromOpaque;

	// HDR tone mapping with gamma correction
	outColor.xyz = vec3(1.02, 1.01, 1.0) - exp(-outColor.xyz * Exposure);
	outColor.xyz = pow(outColor.xyz, vec3(1.0 / CameraGamma));
}