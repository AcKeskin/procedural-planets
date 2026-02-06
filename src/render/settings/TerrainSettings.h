#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace planets::render {

// Earth terrain settings (balanced defaults for Earth-like appearance)
struct EarthTerrainSettings
{
    // Continental noise (large-scale landmass shapes)
    int continentOctaves = 6;
    float continentScale = 0.8f;
    float continentPersistence = 0.5f;
    float continentLacunarity = 2.0f;
    float continentStrength = 2.0f;
    float continentBaseLevel = -0.35f;

    // Mountain ridge noise (sharp peaks)
    int mountainOctaves = 5;
    float mountainScale = 1.5f;
    float mountainPersistence = 0.5f;
    float mountainLacunarity = 4.0f;
    float mountainStrength = 0.87f;
    float mountainPower = 2.18f;
    float mountainGain = 0.8f;
    float mountainSmoothing = 1.0f;

    // Mask noise (controls where mountains appear)
    int maskOctaves = 3;
    float maskScale = 1.09f;
    float maskPersistence = 0.55f;
    float maskLacunarity = 1.66f;

    // Ocean parameters
    float oceanDepthMultiplier = 5.0f;
    float oceanFloorDepth = 1.36f;
    float oceanFloorSmoothing = 0.5f;
    float mountainBlend = 1.16f;

    // Overall scaling
    float heightScale = 0.04f;
    float globalFrequency = 1.0f;

    // Vertex perturbation (micro-detail roughness)
    float perturbStrength = 0.002f;
    float perturbScale = 20.0f;
};

// Shading noise settings for Earth-like surface detail
struct EarthShadingSettings
{
    float biomeNoiseScale = 0.5f;
    float detailNoiseScale = 2.0f;
    float fineNoiseScale = 8.0f;
    float warpStrength = 0.3f;

    float largeNoiseScale = 0.3f;
    int largeNoiseOctaves = 3;

    float smallNoiseScale = 15.0f;
    int smallNoiseOctaves = 5;

    float flatColBlend = 1.5f;
    float flatColBlendNoise = 0.3f;
};

// Generation parameters (seed, subdivisions, GPU toggle)
struct GenerationConfig
{
    uint32_t seed = 42;
    int subdivisions = 6;
    bool useGpu = true;
};

// LOD system configuration
struct LodConfig
{
    bool enabled = true;
    int patchSubdivisions = 2;
    float planetRadius = 10.0f;
};

// Read-only terrain statistics for display
struct TerrainStats
{
    int patchCount = 0;
    int visiblePatchCount = 0;
    int vertexCount = 0;
    float cpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    bool gpuAvailable = false;
};

} // namespace planets::render
