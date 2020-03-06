#version 430

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D AlbedoTexture;

layout(std140, binding = 16) uniform MaterialNumericalProperties
{
	vec4  AlbedoColor;
	float Anisotropy;
	float Roughness;
};

// TODO: replace Lambertian model with PBR
const vec3  LightDirection = vec3(0.0f, 0.8660254f, -0.5f);
const vec4  LightColor = vec4(1, 1, 1, 1);
const float LightIntensity = 1.5f;
const float AmbientIntensity = 0.05f;


void main(void)
{
	vec4 colorFromAlbedoTexture = texture(AlbedoTexture, v2fTexCoord);
	if (colorFromAlbedoTexture.a <= 0)
	{
		discard;
	}

	outColor = AlbedoColor * colorFromAlbedoTexture * LightColor * (clamp(dot(v2fNormal, LightDirection), 0.0f, 1e10) + AmbientIntensity) * LightIntensity; // 0.1f is ambient intensity
}