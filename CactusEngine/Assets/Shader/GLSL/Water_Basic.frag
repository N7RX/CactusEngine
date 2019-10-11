#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;
in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;

uniform vec3 CameraPosition;

// TODO: replace Phong model with PBR
uniform vec3  LightPosition = vec3(0.5f, 0.5f, 2.5f);
uniform vec4  LightColor = vec4(1, 1, 1, 1);
uniform float LightIntensity = 2.0f;

uniform float Ka = 0.25f;
uniform float Kd = 0.9f;
uniform float Ks = 1.0f;
uniform int Shininess = 50;

uniform float FoamHeight = -1.1f;
uniform float FoamRange = 0.1f; // This is determined by Amplitude in previous stage
uniform vec4  FoamColor = vec4(1, 1, 1, 1);

void main(void)
{
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);

	// TODO: replace Phong model with PBR

	vec3 lightDir = normalize(LightPosition - v2fPosition);
	vec3 viewDir = normalize(CameraPosition - v2fPosition);
	vec3 reflectDir = normalize(2 * dot(lightDir, v2fNormal) * v2fNormal - lightDir);

	float lightFalloff = 1.0f / pow(length(LightPosition - v2fPosition), 2);
	float illumination = LightIntensity * (Ka + lightFalloff * (Kd * dot(lightDir, v2fNormal) + Ks * pow(dot(viewDir, reflectDir), Shininess)));

	// Foam approximation
	vec4 waterColor = (AlbedoColor * colorFromAlbedoTexture * LightColor);
	float foamFactor = pow(10.0f * clamp(v2fPosition.y - FoamHeight, 0, FoamRange), 2);
	waterColor = foamFactor * FoamColor + (1.0f - foamFactor) * waterColor;

	vec4 finalColor = vec4(waterColor.xyz * illumination, waterColor.a);

	outColor = finalColor;
}