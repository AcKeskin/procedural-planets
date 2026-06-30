// Cook-Torrance PBR BRDF — metallic/roughness workflow.
// Used by the realistic (PBR) planet fragment path. Single directional sun + ambient.

#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef PI
#define PI 3.14159265359
#endif

// GGX / Trowbridge-Reitz normal distribution
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-5);
}

// Smith geometry with Schlick-GGX, direct-lighting k
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float ggxV = geometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    float ggxL = geometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    return ggxV * ggxL;
}

// Fresnel-Schlick
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// One directional light. Returns outgoing radiance (no ambient — caller adds it).
// lightDir points FROM surface TOWARD the light (already normalized).
vec3 cookTorrance(vec3 albedo, float metallic, float roughness,
                  vec3 N, vec3 V, vec3 lightDir, vec3 lightColor)
{
    vec3 H = normalize(V + lightDir);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, V, lightDir, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, lightDir), 0.0) + 1e-4;
    vec3 specular = numerator / denom;

    vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, lightDir), 0.0);
    return (kd * albedo / PI + specular) * lightColor * NdotL;
}

#endif // PBR_GLSL
