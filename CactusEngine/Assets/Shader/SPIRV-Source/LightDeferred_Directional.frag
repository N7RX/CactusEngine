#version 430

layout(location = 0) in vec2 v2fTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 20) uniform sampler2D GColorTexture;


void main(void)
{
	// Currently directional light is handled in opaque node, and here is just a pass through
	outColor = texture(GColorTexture, v2fTexCoord);
}