#version 450 core

in vec2 v_TexCoords;

out vec2 o_Color;

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

float GeometrySchlick(float NdotV, float roughness)
{
    float a = roughness;
    float k = a * a / 2.0;      // For IBL, as prescribed by Disney and UE

    float num = NdotV;
    float denom = k + NdotV * (1.0 - k);

    return num / max(denom, 0.00001);
}

float GeometrySmith(vec3 N, vec3 L, vec3 V, float roughness)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float ggx1 = GeometrySchlick(NdotL, roughness);
    float ggx2 = GeometrySchlick(NdotV, roughness);

    return ggx1 * ggx2;
}

void main()
{
    vec3 N = vec3(0.0, 0.0, 1.0);

    float NdotV = v_TexCoords.x;
    float roughness = v_TexCoords.y;

    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    // BRDF = F0 * A + B
    float A = 0.0;
    float B = 0.0;
    
    // Generate a quasi-random sample vector focused on V
    // Use that to sample and average (weighted) the environment map
    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 X_i = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(X_i, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmith(N, L, V, roughness);
            float G_Vis = G * VdotH / NdotH * NdotV;
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    o_Color = vec2(A, B);
}