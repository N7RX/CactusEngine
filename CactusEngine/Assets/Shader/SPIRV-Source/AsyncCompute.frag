#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outResults;

layout(binding = 6) uniform sampler2D ColorTexture_1;

layout(std140, binding = 18) uniform SystemVariables
{
	float Time;
};


float PseudoRand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 UpdatePosition(vec4 origin, vec2 seed)
{
	vec4 result = origin;

	float coeff = 0.1f * PseudoRand(seed);

	vec4 pressureVal = vec4(0.0);
	for (int i = 0; i < 5000; i += 2)
	{
		pressureVal += 0.001f * vec4(
			PseudoRand(vec2(cos(PseudoRand(vec2(i, origin.z))) * sin(PseudoRand(vec2(i, origin.y))), seed.y)),
			PseudoRand(vec2(sin(PseudoRand(vec2(i, origin.x))) * cos(PseudoRand(vec2(i, origin.z))), seed.y)),
			PseudoRand(vec2(seed.y, cos(PseudoRand(vec2(i, origin.y))) * sin(PseudoRand(vec2(i, origin.x))))),
			0);
	}
	for (int i = 1; i < 5000; i += 2)
	{
		pressureVal -= 0.001f * vec4(
			PseudoRand(vec2(cos(PseudoRand(vec2(i, origin.z))) * sin(PseudoRand(vec2(i, origin.y))), seed.y)),
			PseudoRand(vec2(sin(PseudoRand(vec2(i, origin.x))) * cos(PseudoRand(vec2(i, origin.z))), seed.y)),
			PseudoRand(vec2(seed.y, cos(PseudoRand(vec2(i, origin.y))) * sin(PseudoRand(vec2(i, origin.x))))),
			0);
	}

	result += coeff * pressureVal;

	return result;
}

void main(void)
{
	ivec2 dimension = textureSize(ColorTexture_1, 0);
	ivec2 itexCoord = ivec2(v2fTexCoord.x * float(dimension.x), v2fTexCoord.y * float(dimension.y));

	outResults = UpdatePosition(texelFetch(ColorTexture_1, itexCoord, 0), vec2(float(itexCoord.x + (itexCoord.y - 1) * dimension.x), Time));
}