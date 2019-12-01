#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;
in vec3 v2fPosition;

layout(location = 0) out vec4 outColor;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

uniform vec3 CameraPosition;

uniform float Time;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;

uniform sampler2D DepthTexture_1;
uniform sampler2D ColorTexture_1;

// TODO: replace Phong model with PBR
uniform vec3  LightDirection = vec3(0.0f, 0.8660254f, -0.5f);
uniform vec4  LightColor = vec4(1, 1, 1, 1);
uniform float LightIntensity = 1.5f;

uniform float Ka = 0.25f;
uniform float Kd = 0.9f;
uniform float Ks = 1.0f;
uniform int   Shininess = 64;

uniform float FoamHeight = 0.15f;
uniform float FoamRange = 0.09f;
uniform vec4  FoamColor = vec4(1, 1, 1, 1);

uniform float BlendDepthRange = 0.18f;

// TODO: Pass in camera parameters
uniform float CameraZFar = 1000.0f;
uniform float CameraZNear = 0.3f;

uniform float DistortionAmount = 0.005f;
uniform float DistortionFreq = 2.0f;
uniform float DistortionDensity = 10.0f;


void main(void)
{
	// Screen space sample coordinates
	vec2 screenCoord = (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).xy / (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).w * 0.5f + 0.5f;
	// Simulate refraction
	vec2 distortedCoord = screenCoord + DistortionAmount * vec2(sin(DistortionFreq * Time + DistortionDensity * v2fPosition.x), cos(DistortionFreq * Time + DistortionDensity * v2fPosition.z));

	// Acquire Z-depth
	// Ref: http://web.archive.org/web/20130416194336/http://olivers.posterous.com/linear-depth-in-glsl-for-real
	float backgroundDepth_linear = texture2D(DepthTexture_1, distortedCoord).r;
	float backgroundDepth_env = (2.0f * CameraZNear * CameraZFar) / (CameraZNear + CameraZFar - (2.0f * backgroundDepth_linear - 1.0f) * (CameraZFar - CameraZNear));
	float fragDepth_env = (2.0f * CameraZNear * CameraZFar) / (CameraZNear + CameraZFar - (2.0f * gl_FragCoord.z - 1.0f) * (CameraZFar - CameraZNear));
	if (backgroundDepth_env < fragDepth_env)
	{
		discard;
	}

	// Color with foam approximation
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);
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
	waterColor.xyz = (1.0f - waterColor.a) * texture2D(ColorTexture_1, distortedCoord).xyz + waterColor.a * waterColor.xyz;

	outColor = vec4(waterColor.xyz, 1.0f);
}