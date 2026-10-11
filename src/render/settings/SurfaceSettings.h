#pragma once

#include <glm/glm.hpp>

namespace planets::render
{

// How a planet type's ground looks on top of the shared surface shader: tints for the rock,
// sand and snow materials (1 = Earth). Set from the body's render block on switch.
struct SurfaceSettings
{
    glm::vec3 rockTint = glm::vec3(1.0f);
    glm::vec3 sandTint = glm::vec3(1.0f);
    glm::vec3 snowTint = glm::vec3(1.0f);

    // Climate traits (0 = Earth's latitude climate)
    float polarCaps = 0.0f;    // share of each hemisphere under ice, from the pole (0.2 = small caps)
    float substellar = 0.0f;   // 1 = tidally locked: hot under the sun, frozen on the night side
    float cityCoverage = 0.0f; // 1 = city lights on all land not under snow, not only habitable land
};

} // namespace planets::render
