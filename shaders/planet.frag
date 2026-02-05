#version 450 core

#include "math.glsl"

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;
in vec4 vShadingData; // x=large, y=detail, z=small, w=biomeBlend

out vec4 FragColor;

// Scene uniforms
uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

// Biome control
uniform bool uUseBiomes;
uniform float uSteepnessThreshold;
uniform float uFlatToSteepBlend;
uniform float uSnowLatitude;
uniform float uSnowBlend;
uniform float uSnowLine;
uniform float uShoreHeight;

// Color uniforms (gradient pairs)
uniform vec3 uShoreLow;
uniform vec3 uShoreHigh;
uniform vec3 uFlatLowA;
uniform vec3 uFlatHighA;
uniform vec3 uFlatLowB;
uniform vec3 uFlatHighB;
uniform vec3 uSteepLow;
uniform vec3 uSteepHigh;
uniform vec3 uSnowColor;

// Color blending parameters
uniform float uFlatColBlend;
uniform float uFlatColBlendNoise;

// Height range for normalization
uniform vec2 uHeightMinMax;

// Lighting controls
uniform float uSunIntensity;
uniform float uAmbientLight;
uniform float uSpecularStrength;
uniform float uSpecularPower;

// Ocean colors (fallback for underwater terrain)
const vec3 DEEP_OCEAN = vec3(0.02, 0.05, 0.15);
const vec3 SHALLOW_OCEAN = vec3(0.05, 0.15, 0.35);

// Calculate steepness from sphere normal vs terrain normal
// Returns 0 for flat terrain, 1 for vertical cliff
float calcSteepness(vec3 worldPos, vec3 normal)
{
    vec3 sphereNormal = normalize(worldPos);

    // Guard against degenerate normals (zero or NaN)
    float normalLen = length(normal);
    if (normalLen < 0.001)
    {
        return 0.0; // Treat degenerate normals as flat terrain
    }

    vec3 terrainNormal = normal / normalLen;

    // Handle potentially inverted normals (pointing inward instead of outward)
    // If normal points away from sphere center, flip it
    float dotWithSphere = dot(sphereNormal, terrainNormal);
    if (dotWithSphere < 0.0)
    {
        terrainNormal = -terrainNormal;
        dotWithSphere = -dotWithSphere;
    }

    float dotProduct = clamp(dotWithSphere, 0.0, 1.0);
    return 1.0 - dotProduct;
}

// Get terrain color - simple height-based zones from sea level
vec3 getTerrainColor(vec3 worldPos, vec3 normal, float height)
{
    // Noise for variation (fallback to 0.5 if not generated)
    float noise = vShadingData.x > 0.001 ? vShadingData.x : 0.5;
    float detailNoise = vShadingData.y > 0.001 ? vShadingData.y : 0.5;

    // Core metrics
    float steepness = calcSteepness(worldPos, normal);
    float latitude = abs(normalize(worldPos).y);

    // Height normalized from sea level (0) to max terrain (1)
    // This is the PRIMARY driver of biome coloring
    float h = clamp((height - uSeaLevel) / (uHeightMinMax.y - uSeaLevel), 0.0, 1.0);

    // === HEIGHT-BASED COLOR ZONES ===
    // Zone thresholds (relative to height above sea level)
    float shoreEnd = uShoreHeight;
    float lowlandEnd = 0.35;
    float midlandEnd = 0.65;
    float highlandEnd = uSnowLine;

    // Shore (beach/sand) - near sea level
    vec3 shoreColor = mix(uShoreLow, uShoreHigh, h + (noise - 0.5) * 0.2);

    // Lowland (plains/grassland) - low elevation
    vec3 lowColor = mix(uFlatLowA, uFlatHighA, h + (detailNoise - 0.5) * 0.3);

    // Midland (forest) - mid elevation
    vec3 midColor = mix(uFlatLowB, uFlatHighB, h + (detailNoise - 0.5) * 0.3);

    // Highland (rock/mountain) - high elevation
    vec3 highColor = mix(uSteepLow, uSteepHigh, h);

    // Snow - highest peaks
    vec3 snowColor = uSnowColor;

    // === BLEND BETWEEN ZONES BASED ON HEIGHT ===
    vec3 flatColor;

    if (h < shoreEnd)
    {
        // Shore zone
        float t = h / shoreEnd;
        flatColor = mix(shoreColor, lowColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < lowlandEnd)
    {
        // Lowland zone
        float t = (h - shoreEnd) / (lowlandEnd - shoreEnd);
        flatColor = mix(lowColor, midColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < midlandEnd)
    {
        // Midland zone
        float t = (h - lowlandEnd) / (midlandEnd - lowlandEnd);
        flatColor = mix(midColor, highColor, smoothstep(0.7, 1.0, t));
    }
    else if (h < highlandEnd)
    {
        // Highland zone
        float t = (h - midlandEnd) / (highlandEnd - midlandEnd);
        flatColor = mix(highColor, snowColor, smoothstep(0.8, 1.0, t));
    }
    else
    {
        // Snow zone
        flatColor = snowColor;
    }

    // === STEEPNESS: Rock on steep slopes ===
    // Only apply rock color where terrain is actually steep
    vec3 rockColor = mix(uSteepLow, uSteepHigh, h * 0.5 + 0.25);
    float rockBlend = smoothstep(uSteepnessThreshold - uFlatToSteepBlend,
                                  uSteepnessThreshold + uFlatToSteepBlend,
                                  steepness);
    vec3 terrainColor = mix(flatColor, rockColor, rockBlend);

    // === LATITUDE: Extra snow at poles ===
    float polarSnow = smoothstep(uSnowLatitude - uSnowBlend, uSnowLatitude, latitude);
    terrainColor = mix(terrainColor, snowColor, polarSnow * 0.8);

    return terrainColor;
}

// Legacy terrain color (height-based only, no biomes)
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
    // Guard against degenerate normals
    float normalLen = length(vNormal);
    vec3 normal = normalLen > 0.001 ? vNormal / normalLen : normalize(vWorldPos);
    vec3 lightDir = -normalize(uLightDir);

    // Diffuse lighting with intensity control
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), uSpecularPower) * uSpecularStrength;

    // Get terrain color
    vec3 terrainColor;
    if (vHeight < uSeaLevel)
    {
        // Underwater (ocean floor visible through shallow water)
        float oceanDepth = remap01(vHeight, uSeaLevel - 0.1, uSeaLevel);
        terrainColor = mix(DEEP_OCEAN, SHALLOW_OCEAN, oceanDepth);
    }
    else if (uUseBiomes)
    {
        // Pass the corrected normal, not raw vNormal
        terrainColor = getTerrainColor(vWorldPos, normal, vHeight);
    }
    else
    {
        terrainColor = getTerrainColorLegacy(vHeight);
    }

    // Final color with lighting (ambient + diffuse * intensity + specular)
    vec3 color = terrainColor * (uAmbientLight + diff * uSunIntensity) + vec3(spec);

    FragColor = vec4(color, 1.0);
}
