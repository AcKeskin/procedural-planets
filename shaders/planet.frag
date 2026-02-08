#version 450 core

#include "math.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // x=large, y=detail, z=small, w=biomeBlend

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

uniform bool uUseBiomes;
uniform float uSteepnessThreshold;
uniform float uFlatToSteepBlend;
uniform float uSnowLatitude;
uniform float uSnowBlend;
uniform float uSnowLine;
uniform float uShoreHeight;

uniform vec3 uShoreLow;
uniform vec3 uShoreHigh;
uniform vec3 uFlatLowA;
uniform vec3 uFlatHighA;
uniform vec3 uFlatLowB;
uniform vec3 uFlatHighB;
uniform vec3 uSteepLow;
uniform vec3 uSteepHigh;
uniform vec3 uSnowColor;

uniform float uFlatColBlend;
uniform float uFlatColBlendNoise;

uniform vec2 uHeightMinMax;

uniform float uSunIntensity;
uniform float uAmbientLight;
uniform float uSpecularStrength;
uniform float uSpecularPower;

// Distance-adaptive fresnel for planetary scale perception
uniform float uPlanetScale;
uniform float uFresnelStrengthNear;
uniform float uFresnelStrengthFar;
uniform float uFresnelPow;

const vec3 DEEP_OCEAN = vec3(0.02, 0.05, 0.15);
const vec3 SHALLOW_OCEAN = vec3(0.05, 0.15, 0.35);

// 0 = flat, 1 = vertical cliff
float calcSteepness(vec3 worldPos, vec3 normal)
{
    vec3 sphereNormal = normalize(worldPos);

    float normalLen = length(normal);
    if (normalLen < 0.001)
        return 0.0;

    vec3 terrainNormal = normal / normalLen;

    // Flip inward-pointing normals
    float dotWithSphere = dot(sphereNormal, terrainNormal);
    if (dotWithSphere < 0.0)
    {
        terrainNormal = -terrainNormal;
        dotWithSphere = -dotWithSphere;
    }

    return 1.0 - clamp(dotWithSphere, 0.0, 1.0);
}

// Height-based biome zones with noise variation
vec3 getTerrainColor(vec3 worldPos, vec3 normal, float height)
{
    float noise = vShadingData.x > 0.001 ? vShadingData.x : 0.5;
    float detailNoise = vShadingData.y > 0.001 ? vShadingData.y : 0.5;

    float steepness = calcSteepness(worldPos, normal);
    float latitude = abs(normalize(worldPos).y);

    // Height normalized: 0 = sea level, 1 = max terrain
    float h = clamp((height - uSeaLevel) / (uHeightMinMax.y - uSeaLevel), 0.0, 1.0);

    float shoreEnd = uShoreHeight;
    float lowlandEnd = 0.35;
    float midlandEnd = 0.65;
    float highlandEnd = uSnowLine;

    vec3 shoreColor = mix(uShoreLow, uShoreHigh, h + (noise - 0.5) * 0.2);
    vec3 lowColor = mix(uFlatLowA, uFlatHighA, h + (detailNoise - 0.5) * 0.3);
    vec3 midColor = mix(uFlatLowB, uFlatHighB, h + (detailNoise - 0.5) * 0.3);
    vec3 highColor = mix(uSteepLow, uSteepHigh, h);
    vec3 snowColor = uSnowColor;

    // Blend between height zones
    vec3 flatColor;

    if (h < shoreEnd)
    {
        float t = h / shoreEnd;
        flatColor = mix(shoreColor, lowColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < lowlandEnd)
    {
        float t = (h - shoreEnd) / (lowlandEnd - shoreEnd);
        flatColor = mix(lowColor, midColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < midlandEnd)
    {
        float t = (h - lowlandEnd) / (midlandEnd - lowlandEnd);
        flatColor = mix(midColor, highColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < highlandEnd)
    {
        float t = (h - midlandEnd) / (highlandEnd - midlandEnd);
        flatColor = mix(highColor, snowColor, smoothstep(0.8, 1.0, t));
    }
    else
    {
        flatColor = snowColor;
    }

    // Rock on steep slopes
    vec3 rockColor = mix(uSteepLow, uSteepHigh, h * 0.5 + 0.25);
    float rockBlend = smoothstep(uSteepnessThreshold - uFlatToSteepBlend,
                                  uSteepnessThreshold + uFlatToSteepBlend,
                                  steepness);
    vec3 terrainColor = mix(flatColor, rockColor, rockBlend);

    // Polar snow
    float polarSnow = smoothstep(uSnowLatitude - uSnowBlend, uSnowLatitude, latitude);
    terrainColor = mix(terrainColor, snowColor, polarSnow * 0.8);

    return terrainColor;
}

// Legacy height-only coloring (no biomes)
vec3 getTerrainColorLegacy(float height)
{
    float beachThreshold = uSeaLevel + 0.02;
    float grassThreshold = uSeaLevel + 0.15;
    float forestThreshold = uSeaLevel + 0.30;
    float rockThreshold = uSeaLevel + 0.50;
    float snowThreshold = uSeaLevel + 0.70;

    vec3 beach = vec3(0.76, 0.70, 0.50);
    vec3 grass = vec3(0.45, 0.55, 0.30);
    vec3 forest = vec3(0.15, 0.40, 0.15);
    vec3 rock = vec3(0.40, 0.38, 0.35);
    vec3 snow = vec3(0.95, 0.95, 0.98);

    if (height < uSeaLevel - 0.1)
        return DEEP_OCEAN;
    else if (height < uSeaLevel)
        return mix(DEEP_OCEAN, SHALLOW_OCEAN, remap01(height, uSeaLevel - 0.1, uSeaLevel));
    else if (height < beachThreshold)
        return mix(SHALLOW_OCEAN, beach, remap01(height, uSeaLevel, beachThreshold));
    else if (height < grassThreshold)
        return mix(beach, grass, remap01(height, beachThreshold, grassThreshold));
    else if (height < forestThreshold)
        return mix(grass, forest, remap01(height, grassThreshold, forestThreshold));
    else if (height < rockThreshold)
        return mix(forest, rock, remap01(height, forestThreshold, rockThreshold));
    else if (height < snowThreshold)
        return mix(rock, snow, remap01(height, rockThreshold, snowThreshold));
    else
        return snow;
}

void main()
{
    float normalLen = length(vNormal);
    vec3 normal = normalLen > 0.001 ? vNormal / normalLen : normalize(vWorldPos);
    vec3 lightDir = -normalize(uLightDir);

    float diff = max(dot(normal, lightDir), 0.0);

    // Blinn-Phong specular
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), uSpecularPower) * uSpecularStrength;

    vec3 terrainColor;
    if (vHeight < uSeaLevel)
    {
        float oceanDepth = remap01(vHeight, uSeaLevel - 0.1, uSeaLevel);
        terrainColor = mix(DEEP_OCEAN, SHALLOW_OCEAN, oceanDepth);
    }
    else if (uUseBiomes)
    {
        terrainColor = getTerrainColor(vWorldPos, normal, vHeight);
    }
    else
    {
        terrainColor = getTerrainColorLegacy(vHeight);
    }

    vec3 color = terrainColor * (uAmbientLight + diff * uSunIntensity) + vec3(spec);

    // Distance-adaptive fresnel rim (fades near surface, strong from orbit)
    float camDist = length(uCameraPos - vWorldPos);
    float camRadiiFromSurface = (camDist - uPlanetScale) / uPlanetScale;
    float fresnelT = smoothstep(0.0, 1.0, camRadiiFromSurface);
    float fresStrength = mix(uFresnelStrengthNear, uFresnelStrengthFar, fresnelT);

    vec3 sphereNormal = normalize(vWorldPos);
    float fresnelDot = 1.0 + dot(normalize(vWorldPos - uCameraPos), sphereNormal);
    float fresnel = clamp(fresStrength * pow(max(fresnelDot, 0.0), uFresnelPow), 0.0, 1.0);

    color += vec3(0.4, 0.6, 1.0) * fresnel * 0.3;

    FragColor = vec4(color, 1.0);
}
