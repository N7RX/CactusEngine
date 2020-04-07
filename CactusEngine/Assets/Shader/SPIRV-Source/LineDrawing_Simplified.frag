#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 6) uniform sampler2D ColorTexture_1; // GBuffer position z (g); Tone value (b)


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

	vec2 left  = texelFetch(ColorTexture_1, fragCoord - ivec2(1, 0), 0).gb;
	vec2 right = texelFetch(ColorTexture_1, fragCoord + ivec2(1, 0), 0).gb;
	vec2 above = texelFetch(ColorTexture_1, fragCoord + ivec2(0, 1), 0).gb;
	vec2 below = texelFetch(ColorTexture_1, fragCoord - ivec2(0, 1), 0).gb;

	float zposResult = pow((left.x - right.x), 2) + pow((above.x - below.x), 2);
	float toneResult = pow((left.y - right.y), 2) + pow((above.y - below.y), 2) * 10.0;

	opacity = clamp(zposResult + toneResult, 0.0, 1.0);

	return opacity;
}

void main(void)
{
	float opacity = determineLineOpacity();
	outColor = vec4(opacity);
}