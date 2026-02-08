#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects {

// Ocean rendering settings
struct OceanSettings
{
    bool enabled = true;
    glm::vec3 deepColor = glm::vec3(0.01f, 0.03f, 0.1f);
    glm::vec3 shallowColor = glm::vec3(0.1f, 0.4f, 0.6f);
    float fresnelPower = 2.0f;
    float depthMultiplier = 10.0f;
    float alphaMultiplier = 70.0f;
    float smoothness = 0.92f;
    float waveStrength = 0.15f;
    float waveScale = 15.0f;
    float waveSpeed = 0.5f;
};

} // namespace planets::render::effects
