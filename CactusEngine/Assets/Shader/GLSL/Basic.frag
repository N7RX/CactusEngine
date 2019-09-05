#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;

layout(location = 0) out vec4 outColor;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;


void main(void)
{
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);

	if (colorFromAlbedoTexture.x + colorFromAlbedoTexture.y + colorFromAlbedoTexture.z < 0.001f) // A temp fix before default texture function is implemented
	{
		colorFromAlbedoTexture.xyz = vec3(1.0f, 1.0f, 1.0f);
	}

	outColor = AlbedoColor * colorFromAlbedoTexture;
}