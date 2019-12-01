#version 430

in vec2 v2fTexCoord;

uniform sampler2D AlbedoTexture;


void main(void)
{
	gl_FragDepth = gl_FragCoord.z;

	// Transparent cutout
	vec4 colorFromTexture = texture2D(AlbedoTexture, v2fTexCoord);
	if (colorFromTexture.a <= 0)
	{
		gl_FragDepth = 1;
	}
}