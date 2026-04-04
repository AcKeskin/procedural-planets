#version 450 core

#include "math.glsl"
#include "lighting.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // x=normalizedHeight, y=latitude, z=detailNoise, w=smallNoise

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

uniform float uSunIntensity;
uniform float uAmbientLight;
uniform float uSpecularStrength;
uniform float uSpecularPower;

uniform float uPlanetScale;
uniform float uFresnelStrengthNear;
uniform float uFresnelStrengthFar;
uniform float uFresnelPow;

uniform float uDetailFadeStart;
uniform float uAOStrength;
uniform float uHazeStrength;
uniform vec3 uHazeColor;

// Biome palette SSBO — body type defines entry count and meaning
#define BIOME_PALETTE_BINDING 3

layout(std430, binding = BIOME_PALETTE_BINDING) readonly buffer BiomePaletteBuffer
{
    vec4 paletteEntries[]; // xyz=color, w=parameter (height threshold)
};

uniform int uPaletteSize;

// Map normalized height to palette color using height thresholds in parameter field
// Entries are sorted by ascending parameter (threshold)
vec3 getPaletteColor(float normalizedHeight, float detailNoise, float detailFade)
{
    if (uPaletteSize <= 0)
        return vec3(0.5);

    if (uPaletteSize == 1)
        return paletteEntries[0].xyz;

    // Find the two palette entries that bracket this height
    // parameter field = height threshold for this color
    int lower = 0;
    int upper = 0;

    for (int i = 0; i < uPaletteSize; i++)
    {
        if (paletteEntries[i].w <= normalizedHeight)
            lower = i;
        else
        {
            upper = i;
            break;
        }
        upper = i;
    }

    if (lower == upper)
        return paletteEntries[lower].xyz;

    // Smooth blend between adjacent palette entries
    float lowerThreshold = paletteEntries[lower].w;
    float upperThreshold = paletteEntries[upper].w;
    float range = upperThreshold - lowerThreshold;
    float t = clamp((normalizedHeight - lowerThreshold) / max(range, 0.001), 0.0, 1.0);
    t = smoothstep(0.0, 1.0, t);

    vec3 color = mix(paletteEntries[lower].xyz, paletteEntries[upper].xyz, t);

    // Subtle detail variation
    float detail = (detailNoise - 0.5) * 0.06 * detailFade;
    color += vec3(detail);

    return clamp(color, 0.0, 1.0);
}

// Steepness: 0 = flat, 1 = vertical cliff
float calcSteepness(vec3 worldPos, vec3 normal)
{
    vec3 sphereNormal = normalize(worldPos);
    float normalLen = length(normal);
    if (normalLen < 0.001)
        return 0.0;

    vec3 terrainNormal = normal / normalLen;
    float dotWithSphere = dot(sphereNormal, terrainNormal);
    if (dotWithSphere < 0.0)
        dotWithSphere = -dotWithSphere;

    return 1.0 - clamp(dotWithSphere, 0.0, 1.0);
}

void main()
{
    float normalLen = length(vNormal);
    vec3 normal = normalLen > 0.001 ? vNormal / normalLen : normalize(vWorldPos);
    vec3 lightDir = -normalize(uLightDir);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 sphereNormal = normalize(vWorldPos);

    float camDist = length(uCameraPos - vWorldPos);
    float distRadii = calcDistanceRadii(vWorldPos, uCameraPos, uPlanetScale);
    float detailFade = 1.0 - smoothstep(uDetailFadeStart, uDetailFadeStart * 2.0, distRadii);

    // Shading data from compute
    float normalizedHeight = vShadingData.x;
    float latitude = vShadingData.y;
    float detailNoise = vShadingData.z;
    float smallNoise = vShadingData.w;

    // Color from palette based on height
    vec3 terrainColor = getPaletteColor(normalizedHeight, detailNoise, detailFade);

    // Darken steep slopes slightly
    float steepness = calcSteepness(vWorldPos, normal);
    float steepDarken = 1.0 - steepness * 0.3;
    terrainColor *= steepDarken;

    // Lighting
    float aoFactor = calcSphericalAO(normal, sphereNormal, uAOStrength);
    vec3 color = applyLighting(terrainColor, normal, lightDir, viewDir,
                               uSunIntensity, uAmbientLight, aoFactor,
                               uSpecularPower, uSpecularStrength);

    color += calcFresnel(vWorldPos, uCameraPos, sphereNormal,
                         uPlanetScale, uFresnelStrengthNear, uFresnelStrengthFar, uFresnelPow);

    color = applyHaze(color, uHazeColor, normal, lightDir,
                      camDist, uPlanetScale, uHazeStrength,
                      uAmbientLight, uSunIntensity);

    FragColor = vec4(color, 1.0);
}
