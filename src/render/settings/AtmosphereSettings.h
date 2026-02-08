#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects {

// Physically-based atmosphere scattering parameters
struct AtmosphereSettings
{
    bool enabled = true;

    float atmosphereScale = 0.25f;

    // Rayleigh scattering
    glm::vec3 wavelengths = glm::vec3(700.0f, 530.0f, 460.0f);
    float scatteringStrength = 20.0f;
    float densityFalloff = 4.5f;

    // Mie scattering
    float mieCoefficient = 0.005f;
    float mieAnisotropy = 0.76f;
    float mieDensityFalloff = 4.0f;

    int numInScatteringPoints = 10;
    int numOpticalDepthPoints = 10;

    float intensity = 1.0f;

    float ditherStrength = 0.8f;
    float ditherScale = 4.0f;
};

} // namespace planets::render::effects
