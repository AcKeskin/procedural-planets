#pragma once

#include "settings/SceneSettings.h"
#include "settings/AtmosphereSettings.h"
#include <model/BodyConfig.h>
#include <random>
#include <algorithm>
#include <cmath>

namespace planets::render
{

// Randomizes a body's generation config (+ scene lighting + atmosphere) within safe
// ranges, with a few correlation constraints to avoid unrealistic combinations.
inline void RandomizeBodyParameters(planetgen::BodyConfig& cfg,
                                    SceneSettings& scene,
                                    effects::AtmosphereSettings& atmosphere,
                                    uint32_t& seed)
{
    std::mt19937 rng(std::random_device{}());
    auto rf = [&rng](float min, float max)
    {
        return std::uniform_real_distribution<float>(min, max)(rng);
    };
    auto ri = [&rng](int min, int max)
    {
        return std::uniform_int_distribution<int>(min, max)(rng);
    };

    seed = static_cast<uint32_t>(rng());

    auto& sh = cfg.shape;
    auto& tec = cfg.tectonics;
    auto& ofl = cfg.oceanFloor;
    auto& hd = cfg.heightDetail;
    auto& er = cfg.erosion;
    auto& sd = cfg.shading;

    // --- Tectonics ---
    tec.enabled = true;
    tec.numPlates = ri(8, 18);
    // Land/ocean split biased land-positive (earth = 0.45) so worlds aren't mostly ocean.
    tec.continentalFraction = rf(0.42f, 0.62f);
    tec.boundaryWidth = rf(0.08f, 0.3f);
    tec.convergentMountainScale = rf(0.2f, 1.0f);
    tec.divergentRiftDepth = rf(0.1f, 0.5f);
    tec.coastlineNoise = rf(0.15f, 0.40f); // lower top end keeps coasts solid, not frayed into islands
    tec.plateElevationNoise = rf(0.05f, 0.3f);

    // --- Continental shape ---
    // Strength + base level keep land above sea level (earth = 2.0 / -0.18). Low scale +
    // fewer octaves favour large, contiguous continents over fragmented archipelagos, so
    // each world reads as real landmasses facing the camera (earth scale = 0.8).
    sh.continentNoise.octaves = ri(4, 6);
    sh.continentNoise.scale = rf(0.45f, 0.85f);
    sh.continentNoise.strength = rf(1.7f, 2.6f);
    sh.continentNoise.persistence = rf(0.40f, 0.55f);
    sh.continentNoise.lacunarity = rf(1.8f, 2.3f);
    sh.continentBaseLevel = rf(-0.14f, 0.06f);

    sh.mountainNoise.octaves = ri(4, 6);
    sh.mountainNoise.scale = rf(0.8f, 2.5f);
    sh.mountainPower = rf(1.5f, 3.0f);
    sh.mountainGain = rf(0.6f, 1.2f);
    sh.mountainNoise.lacunarity = rf(2.0f, 5.0f);
    sh.mountainSmoothing = rf(0.5f, 2.0f);
    sh.mountainBlend = rf(0.6f, 2.0f);

    // Lower mask scale = larger continent blobs (earth = 1.09); keep coasts from breaking up.
    sh.maskNoise.octaves = ri(2, 3);
    sh.maskNoise.scale = rf(0.6f, 1.1f);
    sh.maskNoise.persistence = rf(0.35f, 0.55f);
    sh.maskNoise.lacunarity = rf(1.4f, 2.0f);

    sh.oceanDepthMultiplier = rf(2.0f, 7.0f);
    sh.oceanFloorDepth = rf(0.8f, 2.0f);
    sh.oceanFloorSmoothing = rf(0.3f, 1.2f);
    sh.heightScale = rf(0.05f, 0.12f);
    sh.globalFrequency = rf(0.6f, 2.0f);

    // Correlations: mountains subordinate to continents.
    sh.mountainNoise.strength = rf(0.4f, (std::min)(1.3f, sh.continentNoise.strength * 0.7f));
    float mScaleMin = (std::max)(0.8f, sh.continentNoise.scale * 0.7f);
    float mScaleMax = (std::min)(2.5f, sh.continentNoise.scale * 1.3f);
    if (mScaleMin > mScaleMax)
        mScaleMin = mScaleMax;
    sh.mountainNoise.scale = rf(mScaleMin, mScaleMax);

    // --- Ocean floor ---
    ofl.enabled = true;
    ofl.shelfWidth = rf(0.08f, 0.25f);
    ofl.oceanRidgeOctaves = ri(3, 5);
    ofl.oceanRidgeScale = rf(0.5f, 1.5f);
    ofl.oceanRidgeStrength = rf(0.1f, 0.5f);
    ofl.oceanRidgePower = rf(1.5f, 2.5f);
    ofl.oceanRidgeGain = rf(0.6f, 1.0f);
    ofl.trenchOctaves = ri(2, 4);
    ofl.trenchScale = rf(0.8f, 2.5f);
    ofl.trenchDepth = rf(0.15f, 0.6f);
    ofl.abyssalOctaves = ri(3, 5);
    ofl.abyssalScale = rf(1.0f, 3.0f);
    ofl.abyssalStrength = rf(0.03f, 0.15f);

    // --- Height detail ---
    hd.detailLowThreshold = rf(-0.3f, 0.0f);
    hd.detailHighThreshold = rf(0.1f, 0.5f);
    hd.perturbStrengthLow = rf(0.0005f, 0.002f);
    hd.perturbStrengthHigh = rf(0.002f, 0.008f);
    hd.detailOctavesLow = ri(1, 3);
    hd.detailOctavesHigh = ri(3, 7);
    hd.detailPersistence = rf(0.3f, 0.6f);
    hd.detailLacunarity = rf(1.8f, 3.0f);
    hd.perturbScale = rf(10.0f, 30.0f);

    // --- Erosion (~40% of the time) ---
    er.enabled = (ri(0, 9) < 4);
    er.iterations = ri(2, 8);
    er.thermalRate = rf(0.005f, 0.05f);
    er.thermalThreshold = rf(0.005f, 0.03f);
    er.hydraulicRate = rf(0.003f, 0.02f);
    er.depositionRate = rf(0.002f, 0.01f);
    er.evaporationRate = rf(0.05f, 0.3f);

    // --- Shading / biomes ---
    sd.steepnessThreshold = rf(0.15f, 0.45f);
    sd.flatToSteepBlend = rf(0.05f, 0.20f);
    sd.snowLatitude = rf(0.65f, 0.85f);
    sd.snowBlend = rf(0.05f, 0.15f);
    sd.snowLine = rf(0.70f, 0.95f);
    sd.shoreHeight = rf(0.04f, 0.15f);
    sd.coastalDepthRange = rf(0.01f, 0.08f);
    sd.aoStrength = rf(0.1f, 0.5f);
    sd.largeNoiseScale = rf(0.2f, 0.7f);
    sd.largeNoiseOctaves = ri(2, 4);
    sd.smallNoiseScale = rf(8.0f, 25.0f);
    sd.smallNoiseOctaves = ri(3, 6);
    sd.detailNoiseScale = rf(1.0f, 4.0f);
    sd.warpStrength = rf(0.1f, 0.6f);
    sd.flatColBlend = rf(0.8f, 2.5f);
    sd.flatColBlendNoise = rf(0.1f, 0.5f);
    sd.useClimateModel = true;
    sd.temperatureLapseRate = rf(1.0f, 3.0f);
    sd.temperatureExponent = rf(0.4f, 1.2f);
    sd.moistureNoiseScale = rf(0.8f, 2.5f);
    sd.moistureNoiseStrength = rf(0.05f, 0.25f);
    sd.hadleyIntensity = rf(0.5f, 1.5f);
    sd.continentalityStrength = rf(0.1f, 0.5f);

    // --- Atmosphere ---
    atmosphere.atmosphereScale = rf(0.12f, 0.20f);
    atmosphere.scatteringStrength = rf(6.0f, 14.0f);
    atmosphere.densityFalloff = rf(3.0f, 7.0f);
    atmosphere.wavelengths.x = rf(680.0f, 720.0f);
    atmosphere.wavelengths.y = rf(510.0f, 550.0f);
    atmosphere.wavelengths.z = rf(440.0f, 470.0f);
    atmosphere.mieCoefficient = rf(0.002f, 0.015f);
    atmosphere.mieDensityFalloff = rf(2.0f, 6.0f);
    atmosphere.intensity = rf(0.8f, 1.5f);

    // --- Lighting ---
    scene.lighting.sunIntensity = rf(0.8f, 1.6f);
    scene.lighting.ambientLight = rf(0.18f, 0.32f);
    float azimuth = rf(-3.14159f, 3.14159f);
    float elevation = rf(0.1f, 1.2f);
    scene.lightDir = glm::normalize(glm::vec3(
        std::cos(elevation) * std::sin(azimuth), std::sin(elevation), std::cos(elevation) * std::cos(azimuth)));
}

} // namespace planets::render
