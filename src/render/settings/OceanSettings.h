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
    float specularPower = 64.0f;
};

} // namespace planets::render::effects
