#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;

layout(location = 0) out vec4 outColor;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;

// TODO: replace Lambertian model with PBR
uniform vec3  LightDirection = vec3(0.57735027f, 0.57735027f, 0.57735027f);
uniform vec4  LightColor = vec4(1, 1, 1, 1);
uniform float LightIntensity = 1.5f;


void main(void)
{
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);

	outColor = AlbedoColor * colorFromAlbedoTexture * LightColor * dot(v2fNormal, LightDirection) * LightIntensity;

	// For line drawing
	outColor.a = clamp(dot(v2fNormal, LightDirection), 0.01f, 1);
}