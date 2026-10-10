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
};

} // namespace planets::render
