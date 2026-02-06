#pragma once

#include <glm/glm.hpp>

namespace planets::render {

// Biome settings for shader - height zones from sea level
// All height thresholds are RELATIVE (0 = sea level, 1 = max terrain height)
struct BiomeSettings
{
    bool enabled = true;

    float steepnessThreshold = 0.3f;
    float flatToSteepBlend = 0.15f;

    float snowLatitude = 0.8f;
    float snowBlend = 0.1f;
    float snowLine = 0.85f;

    float shoreHeight = 0.08f;

    bool autoHeightRange = true;
    float heightRangeMin = 0.96f;
    float heightRangeMax = 1.04f;
};

// Earth terrain color palette with gradient pairs
struct EarthColors
{
    glm::vec3 shoreLow = glm::vec3(0.76f, 0.70f, 0.50f);
    glm::vec3 shoreHigh = glm::vec3(0.65f, 0.58f, 0.42f);

    glm::vec3 flatLowA = glm::vec3(0.45f, 0.55f, 0.30f);
    glm::vec3 flatHighA = glm::vec3(0.20f, 0.40f, 0.18f);

    glm::vec3 flatLowB = glm::vec3(0.15f, 0.40f, 0.15f);
    glm::vec3 flatHighB = glm::vec3(0.10f, 0.28f, 0.12f);

    glm::vec3 steepLow = glm::vec3(0.40f, 0.38f, 0.35f);
    glm::vec3 steepHigh = glm::vec3(0.30f, 0.28f, 0.26f);

    glm::vec3 snow = glm::vec3(0.95f, 0.95f, 0.98f);
};

} // namespace planets::render
