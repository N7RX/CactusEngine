#version 430

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 14) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
	mat4 NormalMatrix;
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

// TODO: replace Phong model with PBR
const vec3  LightDirection = vec3(0.0f, 0.8660254f, -0.5f);
const vec4  LightColor = vec4(1, 1, 1, 1);
const float LightIntensity = 1.5f;

const float Ka = 0.25f;
const float Kd = 0.9f;
const float Ks = 1.0f;
const int   Shininess = 64;

const float FoamHeight = 0.15f;
const float FoamRange = 0.09f;
const vec4  FoamColor = vec4(1, 1, 1, 1);

const float BlendDepthRange = 0.18f;

// TODO: pass in camera parameters
const float CameraZFar = 1000.0f;
const float CameraZNear = 0.3f;

const float DistortionAmount = 0.005f;
const float DistortionFreq = 2.0f;
const float DistortionDensity = 10.0f;


void main(void)
{
	// Screen space sample coordinates
	vec2 screenCoord = (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).xy / (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).w * 0.5f + 0.5f;
	screenCoord.y = -screenCoord.y;
	// Simulate refraction
	vec2 distortedCoord = screenCoord + DistortionAmount * vec2(sin(DistortionFreq * Time + DistortionDensity * v2fPosition.x), cos(DistortionFreq * Time + DistortionDensity * v2fPosition.z));

	// Acquire Z-depth
	// Ref: http://web.archive.org/web/20130416194336/http://olivers.posterous.com/linear-depth-in-glsl-for-real
	float backgroundDepth_linear = texture(DepthTexture_1, distortedCoord).r;
	float backgroundDepth_env = (2.0f * CameraZNear * CameraZFar) / (CameraZNear + CameraZFar - (2.0f * backgroundDepth_linear - 1.0f) * (CameraZFar - CameraZNear));
	float fragDepth_env = (2.0f * CameraZNear * CameraZFar) / (CameraZNear + CameraZFar - (2.0f * gl_FragCoord.z - 1.0f) * (CameraZFar - CameraZNear));
	if (backgroundDepth_env < fragDepth_env)
	{
		discard;
	}

	// Color with foam approximation
	vec4 colorFromAlbedoTexture = texture(AlbedoTexture, v2fTexCoord);
	vec4 waterColor = (AlbedoColor * colorFromAlbedoTexture * LightColor);
	float foamFactor = pow(10.0f * clamp(v2fPosition.y - FoamHeight, 0, FoamRange), 2);
	waterColor = foamFactor * FoamColor + (1.0f - foamFactor) * waterColor;

	// Depth blending with rim foam
	float rimFactor = clamp(backgroundDepth_env - fragDepth_env, 0, BlendDepthRange) / BlendDepthRange;
	waterColor = (1.0f - rimFactor) * FoamColor + rimFactor * waterColor;
	waterColor.a *= step(0, backgroundDepth_env - fragDepth_env) * rimFactor;

	// Lighting
	// TODO: replace Phong model with PBR
	vec3 viewDir = normalize(CameraPosition - v2fPosition);
	vec3 reflectDir = normalize(2 * dot(LightDirection, v2fNormal) * v2fNormal - LightDirection);
	float illumination = LightIntensity * (Ka + (Kd * dot(LightDirection, v2fNormal) + Ks * pow(clamp(dot(viewDir, reflectDir), 0, 1), Shininess)));

	// Blend with background color
	waterColor.xyz *= illumination;
	waterColor.xyz = (1.0f - waterColor.a) * texture(ColorTexture_1, distortedCoord).xyz + waterColor.a * waterColor.xyz;

	outColor = vec4(waterColor.xyz, 1.0f);
}