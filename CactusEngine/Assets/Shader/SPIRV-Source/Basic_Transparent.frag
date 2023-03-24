#version 430

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 22) uniform CameraMatrices
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
};

layout(std140, binding = 18) uniform SystemVariables
{
	float Time;
};

layout(std140, binding = 17) uniform CameraProperties
{
	vec3  CameraPosition;
	float Aperture;
	float FocalDistance;
	float ImageDistance;
};

layout(binding = 1) uniform sampler2D AlbedoTexture;

layout(std140, binding = 16) uniform MaterialNumericalProperties
{
	vec4  AlbedoColor;
	float Anisotropy;
	float Roughness;
};

layout(binding = 4) uniform sampler2D DepthTexture_1;
layout(binding = 6) uniform sampler2D ColorTexture_1;

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

	vec4 foregroundColor = AlbedoColor * colorFromAlbedoTexture * LightColor * (clamp(dot(v2fNormal, LightDirection), 0.0f, 1e10) + AmbientIntensity) * LightIntensity; // 0.1f is ambient intensity
	foregroundColor.r *= 1.5f; // Alert: this should be removed after the demo reel

	vec2 screenCoord = (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).xy / (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).w * 0.5f + 0.5f;
	screenCoord.y = -screenCoord.y;
	
	vec4 backgroundColor = texture(ColorTexture_1, screenCoord);
	backgroundColor.a = 1.0f;

	outColor = colorFromAlbedoTexture.a * foregroundColor + (1.0f - colorFromAlbedoTexture.a) * backgroundColor;
}