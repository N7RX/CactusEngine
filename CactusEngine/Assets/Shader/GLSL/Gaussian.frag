#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1;
uniform bool Bool_1;

//const float Weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216); // For kernel size 5
const float Weight[3] = float[](0.545455, 0.181818, 0.045455); // For kernel size 3
const int Resolution = 1; // Smaller is higher
const int KernelSize = 3; // Actually "half" kernel size


void main(void)
{
	vec2 texOffset = 1.0 / textureSize(ColorTexture_1, 0);
	vec4 result = texture(ColorTexture_1, v2fTexCoord) * Weight[0]; // Current fragment's contribution

	// Two-pass Gaussian
	if (Bool_1)
	{
		// Horizontal
		for (int i = Resolution; i < KernelSize * Resolution; i += Resolution)
		{
			result += texture2D(ColorTexture_1, v2fTexCoord + vec2(texOffset.x * i, 0.0)) * Weight[i / Resolution];
			result += texture2D(ColorTexture_1, v2fTexCoord - vec2(texOffset.x * i, 0.0)) * Weight[i / Resolution];
		}
	}
	else
	{
		// Vertical
		for (int i = Resolution; i < KernelSize * Resolution; i += Resolution)
		{
			result += texture2D(ColorTexture_1, v2fTexCoord + vec2(0.0, texOffset.y * i)) * Weight[i / Resolution];
			result += texture2D(ColorTexture_1, v2fTexCoord - vec2(0.0, texOffset.y * i)) * Weight[i / Resolution];
		}
	}

	outColor = result;
}