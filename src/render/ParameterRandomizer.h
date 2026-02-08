#pragma once

#include "settings/TerrainSettings.h"
#include "settings/SurfaceSettings.h"
#include "settings/SceneSettings.h"
#include "settings/AtmosphereSettings.h"
#include "settings/OceanSettings.h"
#include <random>
#include <algorithm>
#include <cmath>

namespace planets::render
{

// Randomizes all Earth-like planet parameters within safe ranges.
// Applies correlation constraints to prevent unrealistic combinations.
inline void RandomizeEarthParameters(EarthTerrainSettings& terrain,
                                     EarthShadingSettings& shading,
                                     BiomeSettings& biomes,
                                     SceneSettings& scene,
                                     effects::AtmosphereSettings& atmosphere,
                                     effects::OceanSettings& /*ocean*/,
                                     uint32_t& seed)
{
    std::mt19937 rng(std::random_device{}());

    auto randFloat = [&rng](float min, float max)
    {
        return std::uniform_real_distribution<float>(min, max)(rng);
    };
    auto randInt = [&rng](int min, int max)
    {
        return std::uniform_int_distribution<int>(min, max)(rng);
    };

    // New generation seed
    seed = static_cast<uint32_t>(rng());

    // --- Tectonic plates ---
    terrain.useTectonics = true;
    terrain.numPlates = randInt(8, 18);
    terrain.continentalFraction = randFloat(0.3f, 0.6f);
    terrain.boundaryWidth = randFloat(0.08f, 0.3f);
    terrain.convergentMountainScale = randFloat(0.2f, 1.0f);
    terrain.divergentRiftDepth = randFloat(0.1f, 0.5f);
    terrain.coastlineNoise = randFloat(0.15f, 0.6f);
    terrain.plateElevationNoise = randFloat(0.05f, 0.3f);

    // --- Terrain shape ---
    terrain.continentOctaves = randInt(4, 7);
    terrain.continentScale = randFloat(0.5f, 1.5f);
    terrain.continentStrength = randFloat(0.8f, 2.5f);
    terrain.continentPersistence = randFloat(0.35f, 0.65f);
    terrain.continentLacunarity = randFloat(1.8f, 2.5f);
    terrain.continentBaseLevel = randFloat(-0.6f, -0.1f);

    terrain.mountainOctaves = randInt(4, 6);
    terrain.mountainScale = randFloat(0.8f, 2.5f);
    terrain.mountainPower = randFloat(1.5f, 3.0f);
    terrain.mountainGain = randFloat(0.6f, 1.2f);
    terrain.mountainLacunarity = randFloat(2.0f, 5.0f);
    terrain.mountainSmoothing = randFloat(0.5f, 2.0f);
    terrain.mountainBlend = randFloat(0.6f, 2.0f);

    terrain.maskOctaves = randInt(2, 4);
    terrain.maskScale = randFloat(0.5f, 1.5f);
    terrain.maskPersistence = randFloat(0.35f, 0.65f);
    terrain.maskLacunarity = randFloat(1.4f, 2.2f);

    terrain.oceanDepthMultiplier = randFloat(2.0f, 7.0f);
    terrain.oceanFloorDepth = randFloat(0.8f, 2.0f);
    terrain.oceanFloorSmoothing = randFloat(0.3f, 1.2f);

    // Ocean floor topology
    terrain.useOceanFloor = true;
    terrain.shelfWidth = randFloat(0.08f, 0.25f);
    terrain.oceanRidgeOctaves = randInt(3, 5);
    terrain.oceanRidgeScale = randFloat(0.5f, 1.5f);
    terrain.oceanRidgeStrength = randFloat(0.1f, 0.5f);
    terrain.oceanRidgePower = randFloat(1.5f, 2.5f);
    terrain.oceanRidgeGain = randFloat(0.6f, 1.0f);
    terrain.trenchOctaves = randInt(2, 4);
    terrain.trenchScale = randFloat(0.8f, 2.5f);
    terrain.trenchDepth = randFloat(0.15f, 0.6f);
    terrain.abyssalOctaves = randInt(3, 5);
    terrain.abyssalScale = randFloat(1.0f, 3.0f);
    terrain.abyssalStrength = randFloat(0.03f, 0.15f);

    terrain.heightScale = randFloat(0.02f, 0.08f);
    terrain.globalFrequency = randFloat(0.6f, 2.0f);

    // Height-dependent detail
    terrain.detailLowThreshold = randFloat(-0.3f, 0.0f);
    terrain.detailHighThreshold = randFloat(0.1f, 0.5f);
    terrain.perturbStrengthLow = randFloat(0.0005f, 0.002f);
    terrain.perturbStrengthHigh = randFloat(0.002f, 0.008f);
    terrain.detailOctavesLow = randInt(1, 3);
    terrain.detailOctavesHigh = randInt(3, 7);
    terrain.detailPersistence = randFloat(0.3f, 0.6f);
    terrain.detailLacunarity = randFloat(1.8f, 3.0f);
    terrain.perturbScale = randFloat(10.0f, 30.0f);

    // Erosion (randomly enable ~40% of the time)
    terrain.enableErosion = (randInt(0, 9) < 4);
    terrain.erosionIterations = randInt(2, 8);
    terrain.thermalErosionRate = randFloat(0.005f, 0.05f);
    terrain.thermalThreshold = randFloat(0.005f, 0.03f);
    terrain.hydraulicErosionRate = randFloat(0.003f, 0.02f);
    terrain.depositionRate = randFloat(0.002f, 0.01f);
    terrain.evaporationRate = randFloat(0.05f, 0.3f);

    // Correlation: mountains subordinate to continents
    terrain.mountainStrength = randFloat(0.4f, (std::min)(1.3f, terrain.continentStrength * 0.7f));

    // Correlation: mountain scale proportional to continent scale (+-30%)
    float mScaleMin = (std::max)(0.8f, terrain.continentScale * 0.7f);
    float mScaleMax = (std::min)(2.5f, terrain.continentScale * 1.3f);
    if (mScaleMin > mScaleMax)
        mScaleMin = mScaleMax;
    terrain.mountainScale = randFloat(mScaleMin, mScaleMax);

    // --- Atmosphere ---
    atmosphere.atmosphereScale = randFloat(0.15f, 0.35f);
    atmosphere.scatteringStrength = randFloat(15.0f, 30.0f);
    atmosphere.densityFalloff = randFloat(3.0f, 7.0f);
    atmosphere.wavelengths.x = randFloat(680.0f, 720.0f);
    atmosphere.wavelengths.y = randFloat(510.0f, 550.0f);
    atmosphere.wavelengths.z = randFloat(440.0f, 470.0f);

    atmosphere.mieCoefficient = randFloat(0.002f, 0.015f);
    atmosphere.mieDensityFalloff = randFloat(2.0f, 6.0f);

    atmosphere.intensity = randFloat(0.8f, 1.5f);

    // --- Lighting ---
    scene.lighting.sunIntensity = randFloat(0.6f, 1.8f);

    // Correlation: ambient stays below sun intensity
    scene.lighting.ambientLight = randFloat(0.05f, (std::min)(0.25f, scene.lighting.sunIntensity * 0.25f));

    // Sun direction (azimuth only, keep elevation moderate)
    float azimuth = randFloat(-3.14159f, 3.14159f);
    float elevation = randFloat(0.1f, 1.2f);
    scene.lightDir.x = std::cos(elevation) * std::sin(azimuth);
    scene.lightDir.y = std::sin(elevation);
    scene.lightDir.z = std::cos(elevation) * std::cos(azimuth);
    scene.lightDir = glm::normalize(scene.lightDir);

    // --- Biome thresholds ---
    biomes.steepnessThreshold = randFloat(0.15f, 0.45f);
    biomes.flatToSteepBlend = randFloat(0.05f, 0.20f);
    biomes.snowLatitude = randFloat(0.65f, 0.85f);
    biomes.snowBlend = randFloat(0.05f, 0.15f);
    biomes.snowLine = randFloat(0.70f, 0.95f);
    biomes.shoreHeight = randFloat(0.04f, 0.15f);

    // --- Shading noise ---
    shading.largeNoiseScale = randFloat(0.2f, 0.7f);
    shading.largeNoiseOctaves = randInt(2, 4);
    shading.smallNoiseScale = randFloat(8.0f, 25.0f);
    shading.smallNoiseOctaves = randInt(3, 6);
    shading.detailNoiseScale = randFloat(1.0f, 4.0f);
    shading.warpStrength = randFloat(0.1f, 0.6f);
    shading.flatColBlend = randFloat(0.8f, 2.5f);
    shading.flatColBlendNoise = randFloat(0.1f, 0.5f);

    // Climate model
    shading.useClimateModel = true;
    shading.temperatureLapseRate = randFloat(1.0f, 3.0f);
    shading.temperatureExponent = randFloat(0.4f, 1.2f);
    shading.moistureNoiseScale = randFloat(0.8f, 2.5f);
    shading.moistureNoiseStrength = randFloat(0.05f, 0.25f);
    shading.hadleyIntensity = randFloat(0.5f, 1.5f);
    shading.continentalityStrength = randFloat(0.1f, 0.5f);
}

} // namespace planets::render
