#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace planets::render
{

// Earth terrain settings (balanced defaults for Earth-like appearance)
struct EarthTerrainSettings
{
    // Tectonic plate system
    bool useTectonics = true;
    int numPlates = 12;
    float continentalFraction = 0.45f;
    float boundaryWidth = 0.15f;
    float convergentMountainScale = 0.6f;
    float divergentRiftDepth = 0.3f;
    float coastlineNoise = 0.4f;
    float plateElevationNoise = 0.2f;

    // Continental noise (large-scale landmass shapes, used as interior plate detail when tectonics enabled)
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

    // Ocean floor topology (ridges, trenches, abyssal plains)
    bool useOceanFloor = true;
    float shelfWidth = 0.15f;
    int oceanRidgeOctaves = 4;
    float oceanRidgeScale = 0.8f;
    float oceanRidgeStrength = 0.3f;
    float oceanRidgePower = 2.0f;
    float oceanRidgeGain = 0.8f;
    int trenchOctaves = 3;
    float trenchScale = 1.5f;
    float trenchDepth = 0.4f;
    int abyssalOctaves = 4;
    float abyssalScale = 2.0f;
    float abyssalStrength = 0.1f;

    // Erosion simulation
    bool enableErosion = false;
    int erosionIterations = 5;
    float thermalErosionRate = 0.02f;
    float thermalThreshold = 0.01f;
    float hydraulicErosionRate = 0.01f;
    float depositionRate = 0.005f;
    float evaporationRate = 0.1f;

    // Overall scaling
    float heightScale = 0.04f;
    float globalFrequency = 1.0f;

    // Height-dependent detail (mountains rougher, plains smoother)
    float detailLowThreshold = -0.1f;
    float detailHighThreshold = 0.3f;
    float perturbStrengthLow = 0.001f;
    float perturbStrengthHigh = 0.004f;
    int detailOctavesLow = 2;
    int detailOctavesHigh = 5;
    float detailPersistence = 0.45f;
    float detailLacunarity = 2.2f;
    float perturbScale = 20.0f;
};

// Shading noise settings for Earth-like surface detail
struct EarthShadingSettings
{
    float detailNoiseScale = 2.0f;
    float warpStrength = 0.3f;

    float largeNoiseScale = 0.3f;
    int largeNoiseOctaves = 3;

    float smallNoiseScale = 15.0f;
    int smallNoiseOctaves = 5;

    float flatColBlend = 1.5f;
    float flatColBlendNoise = 0.3f;

    // Climate model (latitude + elevation → temperature/moisture)
    bool useClimateModel = true;
    float temperatureLapseRate = 2.0f;
    float moistureNoiseScale = 1.5f;
    float moistureNoiseStrength = 0.15f;
    float hadleyIntensity = 1.0f;
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
    float planetRadius = 50.0f;

    // Quadtree parameters
    int meshResolution = 32;
    int maxDepth = 8;
    float splitThreshold = 2.0f;
    float hysteresis = 1.3f;
    int maxActivePatches = 400;
    float skirtFraction = 0.02f;
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
