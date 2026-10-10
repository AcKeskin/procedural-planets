#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects
{

// Physically-based atmosphere scattering parameters
struct AtmosphereSettings
{
    bool enabled = true;

    float atmosphereScale = 0.18f;

    // Rayleigh scattering
    glm::vec3 wavelengths = glm::vec3(680.0f, 550.0f, 440.0f);
    float scatteringStrength = 8.0f;
    float densityFalloff = 4.5f;

    // Mie scattering: haze near the ground, grey extinction plus the forward halo around the sun
    float mieCoefficient = 0.4f;
    float mieDensityFalloff = 12.0f;

    int numInScatteringPoints = 10;
    int numOpticalDepthPoints = 10;

    float intensity = 1.5f;

    float ditherStrength = 0.8f;
    float ditherScale = 4.0f;
};

} // namespace planets::render::effects
