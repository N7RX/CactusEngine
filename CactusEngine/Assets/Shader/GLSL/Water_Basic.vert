#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 v2fTexCoord;
layout(location = 1) out vec3 v2fNormal;
layout(location = 2) out vec3 v2fPosition;

layout(std140, binding = 14) uniform TransformMatrices
{
	mat4 ModelMatrix;
	mat4 NormalMatrix;
};

layout(std140, binding = 22) uniform CameraMatrices
{
	mat4 ViewMatrix;
	mat4 ProjectionMatrix;
};

layout(std140, binding = 18) uniform SystemVariables
{
	float Time;
};

layout(binding = 9) uniform sampler2D NoiseTexture_1;

const vec2  NoiseDirection = vec2(1.0f, 1.0f); // This should not be normalized
const float NoiseIntensity = 1.0f;
const float NoiseFrequency = 0.05f;

const float WaveLength = 0.08f;
const float Steepness = 0.5f;
const float Amplitude = 0.06f;
const float Speed = 0.12f;
const vec2  Direction = vec2(-0.7071068f, -0.7071068f);


void main(void)
{
	v2fTexCoord = inTexCoord + vec2(fract(-0.05f * Time));

	vec2 noiseTexCoord = inTexCoord + vec2(fract(NoiseFrequency * Time)) * NoiseDirection;
	float noisedTime = Time + NoiseIntensity * texture(NoiseTexture_1, noiseTexCoord).r;

	// Gerstner Wave

	// Position
	float frequency = 2.0f / WaveLength;
	float phi = Speed * frequency;
	
	float xWave = Steepness * Amplitude * Direction.x * cos(frequency * dot(Direction, inPosition.xy) + phi * noisedTime);
	float yWave = Steepness * Amplitude * Direction.y * cos(frequency * dot(Direction, inPosition.xy) + phi * noisedTime);
	float zWave = Amplitude * sin(frequency * dot(Direction, inPosition.xy) + phi * noisedTime);

	vec3 vPosition = vec3(xWave + inPosition.x, yWave + inPosition.y, zWave + inPosition.z);

	// Normal
	float xNormal = -(Direction.x * frequency * Amplitude * cos(frequency * dot(Direction, vPosition.xy) + phi * noisedTime));
	float yNormal = -(Direction.y * frequency * Amplitude * cos(frequency * dot(Direction, vPosition.xy) + phi * noisedTime));
	float zNormal = 1 - (Steepness * frequency * Amplitude * sin(frequency * dot(Direction, vPosition.xy) + phi * noisedTime));

	v2fNormal = normalize(mat3(NormalMatrix) * vec3(xNormal, yNormal, zNormal));

	v2fPosition = (ModelMatrix * vec4(vPosition, 1.0)).xyz;

	gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(vPosition, 1.0);
}