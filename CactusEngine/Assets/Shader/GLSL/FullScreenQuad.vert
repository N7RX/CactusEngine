#version 430

layout(location = 0) out vec2 v2fTexCoord;

// Full-screen quad
const vec4 quad_pos[4] = vec4[](vec4(-1.0, 1.0, 0.0, 1.0), vec4(-1.0, -1.0, 0.0, 1.0), vec4(1.0, 1.0, 0.0, 1.0), vec4(1.0, -1.0, 0.0, 1.0));
const vec2 quad_tex[4] = vec2[](vec2(0.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0));


void main(void)
{
	gl_Position = quad_pos[gl_VertexID];
	v2fTexCoord = quad_tex[gl_VertexID];
}