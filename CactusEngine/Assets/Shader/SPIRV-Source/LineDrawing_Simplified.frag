#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 6) uniform sampler2D ColorTexture_1; // GBuffer normal (rgb); Tone value (a)


float determineLineOpacity(void)
{
	float opacity = 0.0f;

	ivec2 texSize = textureSize(ColorTexture_1, 0);
	ivec2 fragCoord = ivec2(v2fTexCoord.x * texSize.x, v2fTexCoord.y * texSize.y);

	// Remove window border
	if (fragCoord.x < 1 || fragCoord.x >= texSize.x - 1 || fragCoord.y < 1 || fragCoord.y >= texSize.y - 1)
	{
		return opacity;
	}

	vec4 left  = texelFetch(ColorTexture_1, fragCoord - ivec2(1, 0), 0);
	vec4 right = texelFetch(ColorTexture_1, fragCoord + ivec2(1, 0), 0);
	vec4 above = texelFetch(ColorTexture_1, fragCoord + ivec2(0, 1), 0);
	vec4 below = texelFetch(ColorTexture_1, fragCoord - ivec2(0, 1), 0);

	float toneResult = (pow((left.a - right.a), 2) + pow((above.a - below.a), 2)) * 5.0;

	opacity = clamp(dot(left.rgb - right.rgb, left.rgb - right.rgb) + dot(above.rgb - below.rgb, above.rgb - below.rgb) + toneResult, 0.0, 1.0);

	return opacity;
}

void main(void)
{
	float opacity = determineLineOpacity();
	outColor = vec4(opacity);
}