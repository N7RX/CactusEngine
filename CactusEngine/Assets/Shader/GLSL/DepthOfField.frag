#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1;
uniform sampler2D GPositionTexture;
uniform sampler2D MaskTexture;
uniform sampler2D NoiseTexture;

uniform float Time;

uniform mat4 ViewMatrix;

uniform bool Bool_1;

uniform float FocalDistance = 2.0f;
uniform float Aperture = 4.0f;
uniform float ImageDistance = 2.0f;

const int SampleRadius = 2;
const float Weight[10] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216, 0.007, 0.003, 0.0012, 0.0005, 0.0003);
const float PI = 3.1415926536f;


vec3 LevelBlur(int level)
{
	//return vec3(float(level)*0.1); // Visualize blur level

	vec2 pixelOffset = 1.0 / textureSize(ColorTexture_1, 0);

	if (level <= 0)
	{
		return texture(ColorTexture_1, v2fTexCoord).rgb * 1.025; // 1.025 is an empirical compensation to level-0
	}

	float initWeight = Weight[0];
	for (int i = level; i < 10; i += 1)
	{
		initWeight += 2 * Weight[i]; // Maintain the same color strength
	}
	vec3 result = texture(ColorTexture_1, v2fTexCoord).rgb * initWeight; // current fragment's contribution

	if (Bool_1)
	{
		for (int i = SampleRadius; i < SampleRadius * level; i += SampleRadius)
		{
			result += texture(ColorTexture_1, v2fTexCoord + vec2(pixelOffset.x * i, 0.0)).rgb * Weight[i / SampleRadius];
			result += texture(ColorTexture_1, v2fTexCoord - vec2(pixelOffset.x * i, 0.0)).rgb * Weight[i / SampleRadius];
		}
	}
	else
	{
		for (int i = SampleRadius; i < SampleRadius * level; i += SampleRadius)
		{
			result += texture(ColorTexture_1, v2fTexCoord + vec2(0.0, pixelOffset.y * i)).rgb * Weight[i / SampleRadius];
			result += texture(ColorTexture_1, v2fTexCoord - vec2(0.0, pixelOffset.y * i)).rgb * Weight[i / SampleRadius];
		}
	}

	return result;
}

// Adapted from two-pass Gaussian blur
void main(void)
{
	float sceneDepth = abs((ViewMatrix * vec4(texture(GPositionTexture, v2fTexCoord).xyz, 1.0)).z);

	// Determine which level of blur should be applied
	// Currently 10 is max level
	highp int level = 0;
	// CoC formula from http://fileadmin.cs.lth.se/cs/education/edan35/lectures/12dof.pdf
	float focalLength = sceneDepth * ImageDistance / (sceneDepth + ImageDistance);
	level = int(clamp(abs(Aperture * focalLength * (FocalDistance - sceneDepth) / (sceneDepth * (FocalDistance - focalLength))), 0.0, 10.0f));

	vec3 result = LevelBlur(level);
	outColor = vec4(result, 1.0);

	if (!Bool_1) // Final output (vertical)
	{
		// Apply brush mask
		float maskStrength_1 = texture2D(MaskTexture, v2fTexCoord).r;
		float maskStrength_2 = texture2D(NoiseTexture, v2fTexCoord).r;

		float portion = abs(sin(fract(0.1f * Time) * 2 * PI));
		float finalStrength = portion * maskStrength_1 + (1.0f - portion) * maskStrength_2;

		outColor = (1.0f - finalStrength) * outColor + vec4(1, 0.9f, 0.8f, 1) * finalStrength;
	}
}