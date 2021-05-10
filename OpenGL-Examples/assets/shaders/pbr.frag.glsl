#version 450 core

in VS_OUT
{
	vec3 v_WorldPos;
	vec3 v_Normal;
	vec2 v_TexCoords;
	vec3 v_ViewPos;
	vec3 v_LightPositions[4];
} fs_in;

out vec4 o_Color;

uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

uniform sampler2D u_AlbedoMap;
uniform sampler2D u_HeightMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;

uniform sampler2D u_BRDFLUT;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;

uniform vec3 u_LightColors[4];

uniform bool u_TextureToggle;
uniform bool u_IBL;

uniform float u_Exposure;

const float PI = 3.14159265359;

// Trowbridge-Reitz GGX Normal Distribution Function
float Distribution(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = clamp(NdotH * NdotH, 0.0, 1.0);

	float num = a2;
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return num / max(denom, 0.0000001);
}

// Schlick GGX Geometry
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	float num = NdotV;
	float denum = NdotV * (1.0 - k) + k;

	return num / denum;
}

// Smith Geometry
float Geometry(vec3 N, vec3 L, vec3 V, float roughness)
{
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);

	return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 Fresnel(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec2 ParallaxCalculation(vec2 texCoord, vec3 viewDir)
{
	const float minLayers = 8;
	const float maxLayers = 32;
	float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
	float layerDepth = 1.0 / numLayers;

	float currentLayerDepth = 0.0;
	vec2 P = viewDir.xy / viewDir.z * 0.03;
	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = texCoord;
	float currentDepthMapValue = texture(u_HeightMap, texCoord).r;

	while (currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = 1.0 - texture(u_HeightMap, currentTexCoords).r;
		currentLayerDepth += layerDepth;
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(u_HeightMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}

void main()
{
	vec3 V = normalize(fs_in.v_ViewPos - fs_in.v_WorldPos);
	
	vec3 N;
	vec2 texCoords;
	if (u_TextureToggle)
	{
		texCoords = ParallaxCalculation(fs_in.v_TexCoords, V); 
		N = texture(u_NormalMap, texCoords).rgb * 2.0 - 1.0;
	}
	else
	{
		N = normalize(fs_in.v_Normal);
	}
	
	vec3 R = reflect(-V, N);

	vec3 albedo;
	float metallic;
	float roughness;
	float ao;
	if (u_TextureToggle)
	{
		albedo = pow(texture(u_AlbedoMap, texCoords).rgb, vec3(2.2));
		metallic = texture(u_MetallicMap, texCoords).r;
		roughness = texture(u_RoughnessMap, texCoords).r;
		ao = texture(u_AOMap, texCoords).r;
	}
	else
	{
		albedo = u_Albedo;
		metallic = u_Metallic;
		roughness = u_Roughness;
		ao = u_AO;
	}

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; i++)
	{
		vec3 L = normalize(fs_in.v_LightPositions[i] - fs_in.v_WorldPos);
		vec3 H = normalize(L + V);

		float distance = length(fs_in.v_LightPositions[i] - fs_in.v_WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = u_LightColors[i] * attenuation;

		// Cook-Torrance BRDF
		float NDF = Distribution(N, H, roughness);
		float G = Geometry(N, L, V, roughness);
		vec3 F = Fresnel(clamp(dot(H, V), 0.0, 1.0), F0, roughness);

		vec3 num = NDF * G * F;
		float denom = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
		vec3 specular = num / max(denom, 0.001);

		vec3 k_d = vec3(1.0) - F;
		k_d *= 1.0 - metallic;
		vec3 diffuse = k_d * albedo / PI;

		vec3 BRDF = diffuse + specular;
		float NdotL = max(dot(N, L), 0.0);

		Lo += BRDF * radiance * NdotL; 
	}

	vec3 ambient;
	if (u_IBL)
	{
		// IBL
		vec3 k_s = Fresnel(clamp(dot(N, V), 0.0, 1.0), F0, roughness);
		vec3 k_d = 1.0 - k_s;
		vec3 irradiance = texture(u_IrradianceMap, N).rgb;
		vec3 diffuse = irradiance * albedo;

		const float MAX_REFLECTION_LOD = 4.0;
		vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
		vec2 BRDF = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
		vec3 specular = prefilteredColor * (F0 * BRDF.x + BRDF.y);

		ambient = (k_d * diffuse + specular) * ao;
	}
	else
	{
		ambient = vec3(0.03) * albedo * ao;
	}
	
	vec3 color = ambient + Lo;

	// Tone mapping
	color = vec3(1.0) - exp(-color * u_Exposure);
	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	o_Color = vec4(color, 1.0);
}