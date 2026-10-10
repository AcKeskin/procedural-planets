#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects
{

// Ocean rendering settings
struct OceanSettings
{
    bool enabled = true;
    glm::vec3 deepColor = glm::vec3(0.02f, 0.09f, 0.22f);
    glm::vec3 shallowColor = glm::vec3(0.08f, 0.42f, 0.55f);
    float fresnelPower = 2.0f;
    float waveStrength = 0.15f;
    float waveScale = 15.0f;
    float waveSpeed = 0.5f;
};

} // namespace planets::render::effects
