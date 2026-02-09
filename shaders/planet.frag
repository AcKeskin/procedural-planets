#version 450 core

#include "math.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // Climate: x=temp, y=moisture, z=detail, w=small
                      // Legacy: x=large, y=detail, z=small, w=biomeBlend

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

uniform bool uUseBiomes;
uniform bool uUseClimateModel;
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

// Biome color for a given moisture level at a specific temperature band
// Overlapping tri-blend: dry peaks at 0.0, mid at 0.45, wet at 0.85
vec3 biomeByMoisture(float moisture, vec3 dry, vec3 mid, vec3 wet, vec3 detailVar)
{
    float wDry = 1.0 - smoothstep(0.15, 0.50, moisture);
    float wMid = smoothstep(0.15, 0.45, moisture) * (1.0 - smoothstep(0.55, 0.85, moisture));
    float wWet = smoothstep(0.50, 0.80, moisture);

    float wSum = wDry + wMid + wWet;
    vec3 color = (dry * wDry + mid * wMid + wet * wWet) / wSum;
    return color + detailVar;
}

// Whittaker biome color from temperature x moisture
// Overlapping weight-based blending eliminates hard band boundaries
vec3 getWhittakerBiome(float temperature, float moisture, float detailNoise, float smallNoise)
{
    // Earth-referenced biome palette
    const vec3 TROPICAL_WET  = vec3(0.05, 0.28, 0.03);
    const vec3 TROPICAL_MID  = vec3(0.28, 0.40, 0.12);
    const vec3 TROPICAL_DRY  = vec3(0.76, 0.62, 0.35);

    const vec3 TEMPERATE_WET = vec3(0.12, 0.35, 0.08);
    const vec3 TEMPERATE_MID = vec3(0.32, 0.42, 0.18);
    const vec3 TEMPERATE_DRY = vec3(0.65, 0.55, 0.30);

    const vec3 BOREAL_WET    = vec3(0.06, 0.20, 0.08);
    const vec3 BOREAL_MID    = vec3(0.22, 0.28, 0.16);
    const vec3 BOREAL_DRY    = vec3(0.42, 0.40, 0.32);

    const vec3 TUNDRA        = vec3(0.48, 0.50, 0.40);
    const vec3 ICE           = vec3(0.92, 0.94, 0.97);

    // Per-channel detail variation (green varies most for vegetation density)
    float dBase = detailNoise - 0.5;
    vec3 dVar = vec3(dBase * 0.08, dBase * 0.12, dBase * 0.04);

    // Color for each temperature band
    vec3 tropical  = biomeByMoisture(moisture, TROPICAL_DRY, TROPICAL_MID, TROPICAL_WET, dVar);
    vec3 temperate = biomeByMoisture(moisture, TEMPERATE_DRY, TEMPERATE_MID, TEMPERATE_WET, dVar);
    vec3 boreal    = biomeByMoisture(moisture, BOREAL_DRY, BOREAL_MID, BOREAL_WET, dVar);
    vec3 polar     = mix(ICE, TUNDRA, smoothstep(0.0, 0.2, moisture));

    // Overlapping weight functions for continuous temperature blending
    float wTropical  = smoothstep(0.50, 0.80, temperature);
    float wTemperate = smoothstep(0.25, 0.55, temperature) * (1.0 - smoothstep(0.60, 0.85, temperature));
    float wBoreal    = smoothstep(0.08, 0.30, temperature) * (1.0 - smoothstep(0.35, 0.60, temperature));
    float wPolar     = 1.0 - smoothstep(0.08, 0.30, temperature);

    float wSum = wTropical + wTemperate + wBoreal + wPolar;
    vec3 color = (tropical * wTropical + temperate * wTemperate + boreal * wBoreal + polar * wPolar) / wSum;

    // Micro-variation from small noise (subtle surface texture breakup)
    float micro = (smallNoise - 0.5) * 0.04;
    color += vec3(micro * 0.5, micro, micro * 0.3);

    return clamp(color, 0.0, 1.0);
}

// Height-based biome zones with noise variation
vec3 getTerrainColor(vec3 worldPos, vec3 normal, float height)
{
    float steepness = calcSteepness(worldPos, normal);
    float latitude = abs(normalize(worldPos).y);
    float h = clamp((height - uSeaLevel) / (uHeightMinMax.y - uSeaLevel), 0.0, 1.0);

    vec3 flatColor;

    if (uUseClimateModel)
    {
        // Climate model: shadingData = (temperature, moisture, detailNoise, smallNoise)
        float temperature = vShadingData.x;
        float moisture = vShadingData.y;
        float detailNoise = vShadingData.z;
        float smallNoise = vShadingData.w;

        // Noise-perturbed biome boundaries to break visual banding
        float perturbedTemp = clamp(temperature + (detailNoise - 0.5) * 0.12, 0.0, 1.0);
        float perturbedMoist = clamp(moisture + (smallNoise - 0.5) * 0.15, 0.0, 1.0);

        flatColor = getWhittakerBiome(perturbedTemp, perturbedMoist, detailNoise, smallNoise);

        // Temperature-aware shore: warm sandy beaches, cold rocky coasts
        float shoreBoundary = uShoreHeight + (detailNoise - 0.5) * uShoreHeight * 0.5;
        float shoreBlend = smoothstep(shoreBoundary * 1.5, 0.0, h);
        shoreBlend *= (1.0 - smoothstep(0.2, 0.4, steepness));
        vec3 warmShore = mix(uShoreLow, uShoreHigh, h + (detailNoise - 0.5) * 0.2);
        vec3 coldShore = mix(vec3(0.45, 0.42, 0.38), vec3(0.35, 0.33, 0.30), h);
        vec3 shoreColor = mix(coldShore, warmShore, smoothstep(0.2, 0.5, temperature));
        flatColor = mix(flatColor, shoreColor, shoreBlend);

        // Snow: combined altitude + temperature model
        float effectiveSnowLine = uSnowLine - (1.0 - temperature) * 0.12;
        float snowBlend = smoothstep(effectiveSnowLine - 0.08, effectiveSnowLine, h);
        float coldSnow = smoothstep(0.06, 0.01, temperature) * 0.4;
        snowBlend = max(snowBlend, coldSnow);
        flatColor = mix(flatColor, uSnowColor, snowBlend);
    }
    else
    {
        // Legacy height-zone biomes
        float noise = vShadingData.x > 0.001 ? vShadingData.x : 0.5;
        float detailNoise = vShadingData.y > 0.001 ? vShadingData.y : 0.5;

        float shoreEnd = uShoreHeight;
        float lowlandEnd = 0.35;
        float midlandEnd = 0.65;
        float highlandEnd = uSnowLine;

        vec3 shoreColor = mix(uShoreLow, uShoreHigh, h + (noise - 0.5) * 0.2);
        vec3 lowColor = mix(uFlatLowA, uFlatHighA, h + (detailNoise - 0.5) * 0.3);
        vec3 midColor = mix(uFlatLowB, uFlatHighB, h + (detailNoise - 0.5) * 0.3);
        vec3 highColor = mix(uSteepLow, uSteepHigh, h);
        vec3 snowColor = uSnowColor;

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
    }

    // Rock on steep slopes
    vec3 rockColor = mix(uSteepLow, uSteepHigh, h * 0.5 + 0.25);
    float rockBlend = smoothstep(uSteepnessThreshold - uFlatToSteepBlend,
                                  uSteepnessThreshold + uFlatToSteepBlend,
                                  steepness);
    vec3 terrainColor = mix(flatColor, rockColor, rockBlend);

    // Polar snow (skip when climate model handles it via temperature)
    if (!uUseClimateModel)
    {
        float polarSnow = smoothstep(uSnowLatitude - uSnowBlend, uSnowLatitude, latitude);
        terrainColor = mix(terrainColor, uSnowColor, polarSnow * 0.8);
    }

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
