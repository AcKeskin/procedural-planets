#pragma once

#include <glm/glm.hpp>

namespace planets::render::effects
{

// Volumetric cloud layer settings. Altitudes are fractions of the planet radius.
struct CloudSettings
{
    bool enabled = true;
    float altitude = 0.025f;           // cloud base above sea level (raised above the terrain automatically)
    float thickness = 0.03f;           // depth of the layer
    float coverage = 0.52f;            // 0 = clear sky, 1 = overcast (0.52 covers about 40% of the disc)
    float density = 300.0f;            // extinction per planet radius in a full cloud
    float speed = 0.15f;               // drift of the weather around the planet
    float scale = 1.0f;                // size of individual clouds (higher = smaller clouds)
    float shadowStrength = 0.5f;       // how much clouds darken the surface below
    glm::vec3 color = glm::vec3(1.0f); // albedo tint: grey storms, yellow toxic clouds
    int viewSteps = 48;                // march steps per pixel: quality vs cost
};

} // namespace planets::render::effects
