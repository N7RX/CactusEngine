#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(binding = 1) uniform sampler2D AlbedoTexture;


void main(void)
{
	gl_FragDepth = gl_FragCoord.z;

	// Transparent cutout
	vec4 colorFromTexture = texture(AlbedoTexture, v2fTexCoord);
	if (colorFromTexture.a <= 0)
	{
		gl_FragDepth = 1;
	}
}