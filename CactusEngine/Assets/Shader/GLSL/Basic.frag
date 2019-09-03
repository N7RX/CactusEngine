#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;

layout(location = 0) out vec4 outColor;

uniform sampler2D AlbedoTexture;
uniform vec4 AlbedoColor;


void main(void)
{   	
//	outColor = texture2D(AlbedoTexture, v2fTexCoord);
//
//	if (outColor.a <= 0.0f)
//	{
//		discard;
//	}

	outColor = AlbedoColor;
}