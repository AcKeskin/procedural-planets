#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects
{

// Cloud shell rendering settings
struct CloudSettings
{
    bool enabled = true;
    float altitude = 0.025f;       // shell height above the surface, as a fraction of planet radius
    float coverage = 0.65f;        // 0 = clear sky, 1 = overcast
    float density = 0.9f;          // opacity multiplier for covered regions
    float speed = 0.15f;           // drift animation speed
    float scale = 3.0f;            // coverage-noise frequency
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float shadowStrength = 0.35f;  // how much clouds darken the surface below
};

} // namespace planets::render::effects
