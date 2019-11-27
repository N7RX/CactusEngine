#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1; // Curvature result (r and b channel)
uniform int LineWidth = 2;

const float CurvatureThreshold = 0.03f;
const float cl = 0.3f;	// Lower bound
const float cu = 1.2f;	// Upper bound


float determineLineOpacity(void)
{
	float opacity = 0.0f;

	// Instead of solving f during search, we query results from the texture

	vec2 texOffset = 1.0f / textureSize(ColorTexture_1, 0);
	vec2 xTexOffset = vec2(texOffset.x, 0);
	vec2 yTexOffset = vec2(0, texOffset.y);

	vec4 sampleCurvature[9];
	int  sampleIndex = 0;
	for (int i = -LineWidth; i < LineWidth + 1; i += LineWidth)
	{
		for (int j = -LineWidth; j < LineWidth + 1; j += LineWidth)
		{
			sampleCurvature[sampleIndex] = texture2D(ColorTexture_1, v2fTexCoord + i*xTexOffset + j*yTexOffset);
			sampleIndex++;
		}
	}

	float countDiff = 0.0f;;
	float centerCuravture = max(abs(sampleCurvature[4].x), abs(sampleCurvature[4].y));

	for (int i = 0; i < 4; ++i)
	{
		countDiff += float(abs(max(abs(sampleCurvature[i].x), abs(sampleCurvature[i].y)) - centerCuravture) >= CurvatureThreshold);
		countDiff += float(abs(max(abs(sampleCurvature[i + 5].x), abs(sampleCurvature[i + 5].y)) - centerCuravture) >= CurvatureThreshold);
	}

	opacity = countDiff / 8.0f;

	return opacity * clamp((centerCuravture - cl) / (cu - cl), 0, 1);
}

void main(void)
{
	float opacity = determineLineOpacity();
	outColor = vec4(opacity);
}