#pragma once

#include <glm/glm.hpp>

namespace planets::render
{

// Sun lighting parameters for terrain shading
struct LightingSettings
{
    float sunIntensity = 1.0f;
    float ambientLight = 0.15f;
    float specularStrength = 0.0f;
    float specularPower = 32.0f;
};

// Scene-level settings: sun, stars, lighting
struct SceneSettings
{
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    float sunSize = 0.05f;
    glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);
    float starDensity = 1.0f;
    LightingSettings lighting;
};

} // namespace planets::render
