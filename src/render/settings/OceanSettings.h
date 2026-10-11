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
    float clarity = 0.25f;      // terrain units of depth before the seabed fades out
    float waveStrength = 0.15f; // slope of the largest wave
    float waveScale = 1.0f;     // wave frequency multiplier, relative to planet size
    float waveSpeed = 0.5f;

    // What the fluid is. Defaults are water; a planet type's render block changes them.
    float emissive = 0.0f;     // glow of the fluid's own colour (lava ~3)
    float crust = 0.0f;        // share of the surface under dark floating crust, lava's glowing cracks between
    float frozen = 0.0f;       // 1 = ice: no waves, opaque, darker cracks
    float reflectivity = 1.0f; // scales the sky reflection (lava barely reflects)
    glm::vec3 foamColor = glm::vec3(0.85f, 0.92f, 0.95f); // surf at the shore (lava: dark cooled rock)
    glm::vec3 crustColor = glm::vec3(0.05f, 0.04f, 0.04f);
};

} // namespace planets::render::effects
