#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;
in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;

uniform sampler2D ColorTexture_1;

// TODO: replace Lambertian model with PBR
uniform vec3  LightDirection = vec3(0.0f, 0.8660254f, -0.5f);
uniform vec4  LightColor = vec4(1, 1, 1, 1);
uniform float LightIntensity = 1.5f;

const float AmbientIntensity = 0.05f;


void main(void)
{
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);
	if (colorFromAlbedoTexture.a <= 0)
	{
		discard;
	}

	vec4 foregroundColor = AlbedoColor * colorFromAlbedoTexture * LightColor * (clamp(dot(v2fNormal, LightDirection), 0.0f, 1e10) + AmbientIntensity) * LightIntensity; // 0.1f is ambient intensity
	foregroundColor.r *= 1.5f; // Alert: this should be removed after the demo reel

	vec2 screenCoord = (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).xy / (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).w * 0.5f + 0.5f;
	vec4 backgroundColor = texture2D(ColorTexture_1, screenCoord);
	backgroundColor.a = 1.0f;

	outColor = colorFromAlbedoTexture.a * foregroundColor + (1.0f - colorFromAlbedoTexture.a) * backgroundColor;
}