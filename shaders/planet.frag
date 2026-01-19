#version 450 core

in vec3 vNormal;
in vec3 vWorldPos;
in float vHeight;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uSeaLevel;

// Terrain colors
const vec3 COLOR_DEEP_OCEAN = vec3(0.02, 0.05, 0.15);
const vec3 COLOR_SHALLOW_OCEAN = vec3(0.05, 0.15, 0.35);
const vec3 COLOR_BEACH = vec3(0.76, 0.70, 0.50);
const vec3 COLOR_GRASS = vec3(0.22, 0.45, 0.15);
const vec3 COLOR_FOREST = vec3(0.13, 0.35, 0.10);
const vec3 COLOR_ROCK = vec3(0.40, 0.38, 0.35);
const vec3 COLOR_SNOW = vec3(0.95, 0.95, 0.98);

vec3 GetTerrainColor(float height, float seaLevel)
{
    // Elevation zones relative to sea level
    float beachThreshold = seaLevel + 0.02;
    float grassThreshold = seaLevel + 0.15;
    float forestThreshold = seaLevel + 0.30;
    float rockThreshold = seaLevel + 0.50;
    float snowThreshold = seaLevel + 0.70;

    if (height < seaLevel - 0.1)
    {
        return COLOR_DEEP_OCEAN;
    }
    else if (height < seaLevel)
    {
        float t = (height - (seaLevel - 0.1)) / 0.1;
        return mix(COLOR_DEEP_OCEAN, COLOR_SHALLOW_OCEAN, t);
    }
    else if (height < beachThreshold)
    {
        float t = (height - seaLevel) / (beachThreshold - seaLevel);
        return mix(COLOR_SHALLOW_OCEAN, COLOR_BEACH, t);
    }
    else if (height < grassThreshold)
    {
        float t = (height - beachThreshold) / (grassThreshold - beachThreshold);
        return mix(COLOR_BEACH, COLOR_GRASS, t);
    }
    else if (height < forestThreshold)
    {
        float t = (height - grassThreshold) / (forestThreshold - grassThreshold);
        return mix(COLOR_GRASS, COLOR_FOREST, t);
    }
    else if (height < rockThreshold)
    {
        float t = (height - forestThreshold) / (rockThreshold - forestThreshold);
        return mix(COLOR_FOREST, COLOR_ROCK, t);
    }
    else if (height < snowThreshold)
    {
        float t = (height - rockThreshold) / (snowThreshold - rockThreshold);
        return mix(COLOR_ROCK, COLOR_SNOW, t);
    }
    else
    {
        return COLOR_SNOW;
    }
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient
    float ambient = 0.15;

    // Get terrain color based on height
    vec3 terrainColor = GetTerrainColor(vHeight, uSeaLevel);

    // Final color
    vec3 color = terrainColor * (ambient + diff * 0.85);

    FragColor = vec4(color, 1.0);
}
