#version 430

layout(location = 0) out vec4 outColor;

layout(binding = 20) uniform sampler2D GColorTexture;
layout(binding = 2)  uniform sampler2D GNormalTexture;
layout(binding = 3)  uniform sampler2D GPositionTexture;
layout(binding = 4)  uniform sampler2D DepthTexture_1;

layout(std140, binding = 21) uniform LightSourceProperties
{
	vec4  Source;
	vec4  Color;
	float Intensity;
	float Radius;
};

layout(std140, binding = 17) uniform CameraProperties
{
	vec3  CameraPosition;
	float Aperture;
	float FocalDistance;
	float ImageDistance;
};

const float PI = 3.1415926536;


void main(void)
{
	ivec2 texelCoord = ivec2(gl_FragCoord.xy);

	// Depth-tested light volumes against scene depth
	float d = texelFetch(DepthTexture_1, texelCoord, 0).r;

	// Do our own depth testing of light volumes here
	if (gl_FragCoord.z < d)
	{
		discard;
	}

	vec3 fragPos = texelFetch(GPositionTexture, texelCoord, 0).xyz;
	float dist = length(fragPos - Source.xyz);

	if (dist > Radius)
	{
		discard;
	}

	vec3 fragColor = texelFetch(GColorTexture, texelCoord, 0).xyz;
	vec3 fragNormal = texelFetch(GNormalTexture, texelCoord, 0).xyz;
	vec3 v = normalize(CameraPosition - fragPos); // View direction
	vec3 lightDirection = normalize(Source.xyz - fragPos);
	vec3 h = normalize(lightDirection + v);
	const float Roughness = 0.75f;

	// Cook-Torrance specular term
	// Fresnel-Schlick
	vec3 F0 = Color.xyz * 0.75f;
	vec3 F_term = F0 + (vec3(1.0) - F0) * pow(1.0 - max(0.0, dot(fragNormal, v)), 5);
	// Distribution factor
	float D_term = exp(-(1.0-pow(max(0.0, dot(fragNormal, h)), 2)) / (pow(max(0.0001, dot(fragNormal, h)), 2)*Roughness*Roughness)) / (4*Roughness*Roughness*pow(max(0.0001, dot(fragNormal, h)), 4));
	// Geometrical attenuation
	float G_term = min(1.0, min(2*max(0.0, dot(fragNormal, h))*max(0.0, dot(fragNormal, v)) / max(0.0001, dot(v, h)), 2*max(0.0, dot(fragNormal, h))*max(0.0, dot(fragNormal, lightDirection)) / max(0.0001, dot(v, h))));
	vec4 specularColor = vec4(F_term, 1.0)*D_term*G_term / (PI*dot(fragNormal, v)) * 0.1f;

	outColor = vec4(((Color.xyz * fragColor + specularColor.xyz) * pow(1.0 - dist / Radius, 2) * clamp(dot(fragNormal, normalize(Source.xyz - fragPos)), 0.0f, 1e10)) * Intensity, 1);
}