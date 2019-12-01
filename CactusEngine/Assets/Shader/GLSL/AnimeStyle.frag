#version 430

in vec2 v2fTexCoord;
in vec3 v2fNormal;
in vec3 v2fPosition;
in vec3 v2fLightSpacePosition;
in mat3 v2fTBNMatrix;
in vec3 v2fTangent;
in vec3 v2fBitangent;

layout(location = 0) out vec4 outColor;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;

uniform vec3 CameraPosition;

uniform sampler2D AlbedoTexture;
uniform sampler2D GNormalTexture;
uniform sampler2D ToneTexture;
uniform sampler2D ShadowMapDepthTexture;

uniform vec4 AlbedoColor;

uniform float Anisotropy = 0.0f;
uniform float Roughness = 0.8f;

uniform vec3  LightDirection = vec3(0.0f, 0.6f, -0.8f);
uniform vec4  LightColor = vec4(1, 1, 1, 1);
uniform float LightIntensity = 1.15f;

// TODO: Pass in camera parameters
uniform float CameraZFar = 1000.0f;
uniform float CameraZNear = 0.3f;

uniform float ZMin = 1.f;

uniform float Tao = 0.9f;
uniform float Beta = 0.5f;
uniform int	  Gamma = 2;

uniform float Mu = 0.3f;
uniform float Lambda = 0.5f;
uniform float Kai = 0.7f;

const float PI = 3.1415926536;


// Based on https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// --------------------------------------------------------
float ComputeShadow(vec4 fragPosLightSpace, vec3 normal)
{
	vec3 lightDir = vec3(0.0f, 0.8660254f, -0.5f);

	// Perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// Transform to [0,1] range
	projCoords = projCoords * 0.5f + 0.5f;

	// Resolve oversampling
	if (projCoords.z > 1.0f)
	{
		return 0.0f;
	}

	// Get closest depth value from light's perspective (using [0,1] range fragPosLightSpace as coords)
	float closestDepth = texture(ShadowMapDepthTexture, projCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;

	// Remove shadow acne
	float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);

	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

	// Percentage-closer filtering
	vec2 texelSize = 1.0 / textureSize(ShadowMapDepthTexture, 0);

	for (int x = -1; x <= 1; ++x) // 3x3 sampling
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(ShadowMapDepthTexture, projCoords.xy + vec2(x, y) * texelSize).r;

			// Check whether current fragment pos is in shadow
			shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
		}
	}
	shadow /= 9.0f;

	return shadow;
}

void main(void)
{
	vec4 colorFromAlbedoTexture = texture2D(AlbedoTexture, v2fTexCoord);
	if (colorFromAlbedoTexture.a <= 0)
	{
		discard;
	}

	// Dynamic stylized shading

	// Reflection direction
	//vec3 r = normalize(2 * dot(LightDirection, v2fNormal) * v2fNormal - LightDirection);

	// Anisotropy
	vec3 v = normalize(CameraPosition - v2fPosition); // View direction
	vec3 h = normalize(LightDirection + v);
	float eta = (dot(LightDirection, v) + 1.0f) / 2.0f;
	float s = 1.0f - eta + eta / (1.0f - Anisotropy);
	if (Anisotropy < 0)
	{
		s = 1.0f / s;
	}
	vec3 hHat = normalize(s * dot(h, v2fTangent) * v2fTangent + dot(h, v2fBitangent) * v2fBitangent / s + dot(h, v2fNormal) * v2fNormal);
	vec3 vHat = 2 * dot(LightDirection, hHat) * hHat - LightDirection;
	vec3 rHat = 2 * dot(v2fNormal, vHat) * v2fNormal - vHat;

	//vec3 dAlpha = normalize(Roughness * v2fNormal + (1.0f - Roughness) * r);
	vec3 dAlpha = normalize(Roughness * v2fNormal + (1.0f - Roughness) * rHat);

	// Surface feature enhancement

	float TaoPrime = Tao;

	// Obtain pixel size
	vec2 texOffset = 1.0f / textureSize(GNormalTexture, 0);
	vec2 xTexOffset = vec2(texOffset.x, 0);
	vec2 yTexOffset = vec2(0, texOffset.y);

	// Screen space sample coordinates
	vec2 screenCoord = (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).xy / (ProjectionMatrix * ViewMatrix * vec4(v2fPosition, 1)).w * 0.5f + 0.5f;

	// Depth gradient based on normal
	vec3 rightNormal = texture2D(GNormalTexture, screenCoord + xTexOffset).xyz;
	vec3 leftNormal = texture2D(GNormalTexture, screenCoord - xTexOffset).xyz;
	vec3 topNormal = texture2D(GNormalTexture, screenCoord + yTexOffset).xyz;
	vec3 bottomNormal = texture2D(GNormalTexture, screenCoord - yTexOffset).xyz;
	vec2 gRight = vec2(-rightNormal.x / rightNormal.z, -rightNormal.y / rightNormal.z);
	vec2 gLeft = vec2(-leftNormal.x / leftNormal.z, -leftNormal.y / leftNormal.z);
	vec2 gTop = vec2(-topNormal.x / topNormal.z, -topNormal.y / topNormal.z);
	vec2 gBottom = vec2(-bottomNormal.x / bottomNormal.z, -bottomNormal.y / bottomNormal.z);

	// Curvature tensor
	mat2 H;
	H[0] = gRight - gLeft;
	H[1] = gTop - gBottom;

	// Eigen value decomposition
	float delta = pow((H[0][0] + H[1][1]), 2) - 4 * (H[0][0]*H[1][1] - H[1][0]*H[0][1]);
	// TODO: need to find better way to remove if statement
	if (delta > 0)
	{
		// Eigenvalues
		float kappaU = ((H[0][0] + H[1][1]) + sqrt(delta)) / 2.0f;
		float kappaV = ((H[0][0] + H[1][1]) - sqrt(delta)) / 2.0f;

		vec3 u = vec3(1, 0, 0);
		vec3 v = vec3(0, 1, 0);
		vec3 z = vec3(0, 0, 1);

		// Eigenvectors
		if (H[0][1] == 0)
		{
			u = vec3(H[1][0], kappaU - H[0][0], 0);
			v = vec3(H[1][0], kappaV - H[0][0], 0);
		}
		else if (H[1][0] == 0)
		{
			u = vec3(kappaU - H[1][1], H[0][1], 0);
			v = vec3(kappaV - H[1][1], H[0][1], 0);
		}

		float meanCurvature = kappaU + kappaV;
		float deltaKappa = kappaU - kappaV;

		// Transform light direction into (u, v, z) reference frame
		mat3 refM;
		refM[0] = u;
		refM[1] = v;
		refM[2] = z;

		vec3 lightDirPrime = refM * LightDirection;

		float kappa = tanh((meanCurvature + Lambda*deltaKappa) * lightDirPrime.x*lightDirPrime.x + (meanCurvature - Lambda*deltaKappa) * lightDirPrime.y*lightDirPrime.y + meanCurvature * lightDirPrime.z*lightDirPrime.z);

		TaoPrime += Mu * tanh(Kai * kappa);
	}

	// Angular parametrization
	float u = clamp(cosh(dot(dAlpha, v2fNormal)) - TaoPrime, 0, PI);

	// Intensity profile function
	float I = pow(clamp(Beta + (1.0f - Beta) * cos(u), 0.0f, 1e10), Gamma);

	// Toon mapping

	float fragDepth = (2.0f * CameraZNear * CameraZFar) / (CameraZNear + CameraZFar - (2.0f * gl_FragCoord.z - 1.0f) * (CameraZFar - CameraZNear));
	float D = clamp(1.0f - log2(fragDepth / ZMin), 0, 1); // Scale factor (r) is set to 2
	vec2 toonCoord = vec2(clamp(dot(v2fNormal, LightDirection), 0.01f, 1), 1);
	vec4 toneColor = texture2D(ToneTexture, toonCoord) * AlbedoColor * LightIntensity * LightColor;

	// Applying shadow map
	float shadowValue = ComputeShadow(vec4(v2fLightSpacePosition, 1.0f), v2fNormal);

	outColor = I * toneColor * colorFromAlbedoTexture * (2.0f - shadowValue);

	// For line drawing
	outColor.a = toonCoord.x;
}