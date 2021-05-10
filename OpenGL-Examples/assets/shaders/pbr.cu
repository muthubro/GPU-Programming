#include <cuda.h>
#include "stb_image/stb_image.h"

struct vec2
{
    double x, y;
};

vec2 operator +(vec2 a, vec2 b)
{
    return vec2(a.x + b.x, a.y + b.y);
}

vec2 operator *(vec2 a, double s)
{
    return vec2(a.x * s, a.y * s);
}

struct vec3
{
    double x, y, z;

    vec3(double val) { x = val; y = val; z = val; }
    vec2 xy() { return vec2(x, y); }
};

vec3 operator +(vec3 a, vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec3 operator -(vec3 a, vec3 b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec3 pow(vec3 a, vec3 p)
{
    return vec3(pow(a.x, p.x), pow(a.y, p.y), pow(a.z, p.z));
}

#define EXP 2.71828

vec3 exp(vec3 vec)
{
    return pow(vec3(EXP), vec);
}

double dot(vec3 a, vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

vec3 mix(vec3 a, vec3 b, double r)
{
    return a + (b - a) * r;
}

double clamp(double a, double l, double h)
{
    return a < l ? l : (a > h ? h : a);
}

struct Image
{
    float* data;
    int width;
    int height;
}

vec3 texture(Image data, vec2 coords)
{
    int pix = coord.y * image.width + coord.x;
    return vec3(data[pix*3], data[pix*3+1], data[pix*3+2]);
}

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

// Smith's Method
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
	vec2 P = viewDir.xy() / viewDir.z * 0.03;
	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = texCoord;
	float currentDepthMapValue = texture(u_HeightMap, texCoord).x;

	while (currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = 1.0 - texture(u_HeightMap, currentTexCoords).x;
		currentLayerDepth += layerDepth;
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(u_HeightMap, prevTexCoords).x - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}

double Length(vec3 vec)
{
    return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

vec3 Normalize(vec3 vec)
{
    vec *= 1.0 / Length(vec);
    return vec;
}

// Cook-Torrance BRDF
__global__ void PBR(Image& img, vec3 viewPos, vec3 worldPos, vec2 texCoord, 
    Image normalMap, vec3 lightColors[4], vec3 lightPositions[4], bool ibl, 
    Image prefliterMap, Image BRDFLUT, double exposure)
{
    vec3 V = Normalize(viewPos - worldPos);

    vec2 texCoords = ParallaxCalculation(texCoord, V);
    vec3 N = texture(normalMap, texCoord) * 2.0 - vec3(1.0, 1.0, 1.0);

    vec3 albedo;
	float metallic;
	float roughness;
	float ao;

    albedo = pow(texture(u_AlbedoMap, texCoords), vec3(2.2));
    metallic = texture(u_MetallicMap, texCoords).x;
    roughness = texture(u_RoughnessMap, texCoords).x;
    ao = texture(u_AOMap, texCoords).x;

    vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; i++)
	{
		vec3 L = Normalize(lightPositions[i] - worldPos);
		vec3 H = Normalize(L + V);

		float distance = Length(lightPositions[i] - worldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

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
	if (ibl)
	{
		// IBL
		vec3 k_s = Fresnel(clamp(dot(N, V), 0.0, 1.0), F0, roughness);
		vec3 k_d = 1.0 - k_s;
		vec3 irradiance = texture(u_IrradianceMap, N);
		vec3 diffuse = irradiance * albedo;

		const float MAX_REFLECTION_LOD = 4.0;
		vec3 prefilteredColor = texture(prefliterMap, R);
		vec2 BRDF = texture(BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).xy;
		vec3 specular = prefilteredColor * (F0 * BRDF.x + BRDF.y);

		ambient = (k_d * diffuse + specular) * ao;
	}
	else
	{
		ambient = vec3(0.03) * albedo * ao;
	}
	
	vec3 color = ambient + Lo;

	// Tone mapping
	color = vec3(1.0) - exp(-color * exposure);
	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	img[blockIdx.x * img.height + blockIdx.y] = color;
}

int Lighting(int* argc, char** argv)
{
    Image img, normalMap, prefilterMap, BRDFLUT;
    bool ibl = atoi(argv[0]);
    double exposure = atod(argv[1]);

    int channels;
    img.data = stbi_loadf("pirate-gold-bl/pirate-gold_albedo.png", &img.width, &img.height, &channels, 3);
    normalMap.data = stbi_loadf("pirate-gold-bl/pirate-gold_normal-ogl.png", &normalMap.width, &normalMap.height, &channels, 0);
    prefilterMap.data = stbi_loadf("Newport_Loft/Newport_Loft_8k.png", &prefilterMap.width, &prefilterMap.height, &channels, 0);
    BRDFLUT.data = stbi_loadf("BRDF_LUT.tga", &img.width, &img.height, &channels, 0);

    vec3 lightPositions[] = {
        vec3(-10.0f,  10.0f, 10.0f),
        vec3(10.0f,  10.0f, 10.0f),
        vec3(-10.0f, -10.0f, 10.0f),
        vec3(10.0f, -10.0f, 10.0f),
    };
    vec3 lightColors[] = {
        vec3(1000.0f, 1000.0f, 1000.0f),
        vec3(300.0f, 300.0f, 300.0f),
        vec3(300.0f, 300.0f, 300.0f),
        vec3(300.0f, 300.0f, 300.0f)
    };

    PBR<<<vec3(img.width, image.height, 1), 1>>>(img, vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0), vec2(0.2, 0.5), 
        img, lightColors, lightPositions, ibl, prefilterMap, BRDFLUT, exposure);

    output = fopen("assets/textures/lighting.png", "wb");
    fprintf(output, (char*)img.data);
    fclose(output);

    return 0;
}