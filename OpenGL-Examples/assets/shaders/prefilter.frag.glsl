#version 450 core

in vec3 v_WorldPos;

out vec4 o_Color;

uniform float u_Roughness;
uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

// 1D low-discrepancy sequence
// https://en.wikipedia.org/wiki/Van_der_Corput_sequence
float VanDerCorput(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

// Low-discrepancy set
// https://en.wikipedia.org/wiki/Low-discrepancy_sequence#Hammersley_set
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), VanDerCorput(i));
}

// Importance sampling to be used with GGX NDF
vec3 ImportanceSampleGGX(vec2 X, vec3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    // Convert the 2D quasi-random X to spherical angles with theta
    // directly proportional to roughness (more roughness => more scattering)
    float phi = 2 * PI * X.x;
    float cosTheta = sqrt((1.0 - X.y) / (1.0 + (a2 - 1.0) * X.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // Convert spherial coordinates to cartesian
    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    // TBN matrix components (up is just a temporary direction)
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    // Tangent-space to world-space
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return sampleVec;
}

// Trowbridge-Reitz GGX Normal Distribution Function
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

void main()
{
    vec3 N = normalize(v_WorldPos);
    vec3 V = N;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);

    // Generate a quasi-random sample vector focused on V
    // Use that to sample and average (weighted) the environment map
    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 X_i = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(X_i, N, u_Roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            // Probability distribution function of the samples
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float D = DistributionGGX(NdotH, u_Roughness);
            float pdf = (D * NdotH) / (4.0 * HdotV);

            float resolution = 512.0;
            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);     // solid angle subtended by one texel
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.001);     // solid angle subtended by the sample

            // Mip level chosen depending on the pdf value
            // https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
            // (Equation 13)
            float mipLevel = u_Roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(u_EnvironmentMap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor /= totalWeight;

    o_Color = vec4(prefilteredColor, 1.0);
}