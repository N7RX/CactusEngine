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

layout(std140, binding = 17) uniform CameraProperties
{
	vec3  CameraPosition;
	float Aperture;
	float FocalDistance;
	float ImageDistance;
};


vec3 LevelBlur(float level)
{
	//return vec3(level*0.1); // Visualize blur level
	
	// Convert to visually acceptable mipmap level; use natural logarithm to map the level
	level = log(level);
	level = clamp(level, 0.0f, 10.0f);

	// Utilize mipmap instead of Gaussian blur
	return textureLod(ColorTexture_1, v2fTexCoord, level).rgb;
}

// Adapted from two-pass Gaussian blur
void main(void)
{
	float sceneDepth = abs((ViewMatrix * vec4(texture(GPositionTexture, v2fTexCoord).xyz, 1.0)).z);

	// Determine which level of blur should be applied
	
	// CoC formula from http://fileadmin.cs.lth.se/cs/education/edan35/lectures/12dof.pdf
	float focalLength = sceneDepth * ImageDistance / (sceneDepth + ImageDistance);
	float level = abs(Aperture * focalLength * (FocalDistance - sceneDepth) / (sceneDepth * (FocalDistance - focalLength)));

	vec3 result = LevelBlur(level);

	outColor = vec4(result, 1.0);
}