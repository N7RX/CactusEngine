#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1; // Blurred tone value (stored in alpha channel)
// Containing the 5x9 sample parameter matrix
uniform sampler2D SampleMatrixTexture;
uniform int LineWidth = 1;


vec2 findCurvature(in vec2 position)
{
	vec2 texOffset = 1.0f / textureSize(ColorTexture_1, 0);
	vec2 xTexOffset = vec2(texOffset.x, 0);
	vec2 yTexOffset = vec2(0, texOffset.y);

	vec4 sampleColor[9];
	int  sampleIndex = 0;
	for (int i = -LineWidth; i < LineWidth + 1; i += LineWidth)
	{
		for (int j = -LineWidth; j < LineWidth + 1; j += LineWidth)
		{
			sampleColor[sampleIndex] = texture2D(ColorTexture_1, position + i*xTexOffset + j*yTexOffset);
			sampleIndex++;
		}
	}

	// 9x1 tone vector T = T1 + T2 + float(t3)
	vec4 T1, T2;
	float t3 = sampleColor[0].a;
	for (int i = 0; i < 4; ++i)
	{
		// Tone value is stored in alpha channel by previous passes
		T2[i] = sampleColor[1+i].a;
		T1[i] = sampleColor[1+i + 4].a;
	}

	// Retrieve 5x9 matrix from texture
	vec2 matrixTexOffset = 1.0f / textureSize(SampleMatrixTexture, 0);
	matrixTexOffset.y = 0;

	vec4 H[12];
	for (int i = 0; i < 12; ++i)
	{
		H[i] = texture2D(SampleMatrixTexture, matrixTexOffset * i);
	}

	float a0 = dot(H[0], T1) + dot(H[1], T2) + H[10][0] * t3;
	float a1 = dot(H[2], T1) + dot(H[3], T2) + H[10][1] * t3;
	float a2 = dot(H[4], T1) + dot(H[5], T2) + H[10][2] * t3;
	float a3 = dot(H[6], T1) + dot(H[7], T2) + H[10][3] * t3;
	float a4 = dot(H[8], T1) + dot(H[9], T2) + H[11][0] * t3;

	mat2 M;
	M[0] = vec2(a0, a1);
	M[1] = vec2(a1, a2);

	vec2 c = vec2(0);

	float delta = pow((M[0][0] + M[1][1]), 2) - 4 * (M[0][0]*M[1][1] - M[1][0]*M[0][1]);
	// TODO: need to find better way to remove if statement
	if (delta > 0)
	{
		mat2 invM = inverse(M);
		c = -0.5 * vec2(invM[0][0] * a3 + invM[1][0] * a4, invM[0][1] * a3 + invM[1][1] * a4);
	}

	return c;
}

void main(void)
{
	outColor = vec4(findCurvature(v2fTexCoord), 0, 0);
}