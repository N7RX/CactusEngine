#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

out vec2 v2fTexCoord;
out vec3 v2fNormal;
out vec3 v2fPosition;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

uniform float Time;

uniform float WaveLength = 0.08f;
uniform float Steepness = 0.2f;
uniform float Amplitude = 0.1f;
uniform float Speed = 0.1f;
uniform vec2  Direction = vec2(0.7071068f, 0.7071068f);


void main(void)
{
	v2fTexCoord = inTexCoord + vec2(fract(0.05f * Time));

	// Gerstner Wave

	// Position
	float frequency = 2.0f / WaveLength;
	float phi = Speed * frequency;
	
	float xWave = Steepness * Amplitude * Direction.x * cos(frequency * dot(Direction, inPosition.xy) + phi * Time);
	float yWave = Steepness * Amplitude * Direction.y * cos(frequency * dot(Direction, inPosition.xy) + phi * Time);
	float zWave = Amplitude * sin(frequency * dot(Direction, inPosition.xy) + phi * Time);

	vec3 vPosition = vec3(xWave + inPosition.x, yWave + inPosition.y, zWave + inPosition.z);

	// Normal
	float xNormal = -(Direction.x * frequency * Amplitude * cos(frequency * dot(Direction, vPosition.xy) + phi * Time));
	float yNormal = -(Direction.y * frequency * Amplitude * cos(frequency * dot(Direction, vPosition.xy) + phi * Time));
	float zNormal = 1 - (Steepness * frequency * Amplitude * sin(frequency * dot(Direction, vPosition.xy) + phi * Time));

	v2fNormal = vec3(xNormal, yNormal, zNormal);


	v2fPosition = (ModelMatrix * vec4(vPosition, 1.0)).xyz;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(vPosition, 1.0);
}