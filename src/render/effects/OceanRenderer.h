#pragma once

#include "../Shader.h"
#include "../settings/OceanSettings.h"
#include <glm/glm.hpp>

namespace planets::render::lod
{
class PlanetQuadTree;
}

namespace planets::render::effects
{

// Per-frame inputs of the ocean pass.
struct OceanFrame
{
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec3 cameraPos{0.0f};
    glm::vec3 lightDir{0.0f, 0.0f, 1.0f};
    glm::vec3 sunColor{1.0f};
    float sunIntensity = 1.0f;
    float ambientLight = 0.1f;
    float time = 0.0f;
    float seaLevel = 1.0f;    // radial multiplier shared with terrain shading and the atmosphere
    float heightScale = 0.04f; // body shape.heightScale: converts radius-unit depth to terrain units

    // Atmosphere parameters for the reflected sky (atmosphereOn false = airless, black sky).
    bool atmosphereOn = false;
    glm::vec3 wavelengths{700.0f, 530.0f, 460.0f};
    float scatteringStrength = 8.0f;
    float atmosphereIntensity = 1.0f;
};

// Draws the ocean over the terrain's own LOD patches, every vertex at sea level, with depth from
// the heights the terrain draws. No mesh or height source of its own.
class OceanRenderer
{
public:
    OceanRenderer() = default;

    OceanRenderer(const OceanRenderer&) = delete;
    OceanRenderer& operator=(const OceanRenderer&) = delete;

    bool Initialize();

    void Render(const OceanFrame& frame, const OceanSettings& settings, const lod::PlanetQuadTree& patches);

    bool IsReady() const { return _initialized; }

private:
    Shader _oceanShader;
    bool _initialized = false;
};

} // namespace planets::render::effects
