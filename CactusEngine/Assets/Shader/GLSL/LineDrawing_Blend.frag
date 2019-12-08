#version 430

in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D ColorTexture_1; // Main alpha source
uniform sampler2D ColorTexture_2;
uniform vec4 LineColor = vec4(0.0f);


void main(void)
{
	float alpha = texture2D(ColorTexture_1, v2fTexCoord).a;
	outColor = LineColor * alpha + (1.0f - alpha) * texture2D(ColorTexture_2, v2fTexCoord);
}