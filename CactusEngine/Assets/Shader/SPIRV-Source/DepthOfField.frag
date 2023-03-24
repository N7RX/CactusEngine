#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 6) uniform sampler2D ColorTexture_1;
layout(binding = 3) uniform sampler2D GPositionTexture;

layout(std140, binding = 22) uniform CameraMatrices
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
};

layout(std140, binding = 19) uniform ControlVariables
{
	int Bool_1;
};

layout(std140, binding = 17) uniform CameraProperties
{
	vec3  CameraPosition;
	float Aperture;
	float FocalDistance;
	float ImageDistance;
};

const int   SampleRadius = 2;
const float Weight[10] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216, 0.007, 0.003, 0.0012, 0.0005, 0.0003);
const float PI = 3.1415926536f;


vec3 LevelBlur(int level)
{
	//return vec3(float(level)*0.1); // Visualize blur level
	//return texture(ColorTexture_1, v2fTexCoord).rgb;

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

	if (Bool_1 == 1)
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
}