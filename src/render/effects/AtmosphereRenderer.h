#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>

namespace planets::render::effects {

// Atmosphere rendering settings
struct AtmosphereSettings
{
    bool enabled = true;
    glm::vec3 color = glm::vec3(0.4f, 0.7f, 1.0f);
    float height = 0.08f;       // Atmosphere thickness as fraction of planet radius
    float densityFalloff = 4.0f;
    float intensity = 0.8f;
};

// Renders atmosphere as a slightly larger sphere with rim glow
class AtmosphereRenderer
{
public:
    AtmosphereRenderer();
    ~AtmosphereRenderer();

    AtmosphereRenderer(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer& operator=(const AtmosphereRenderer&) = delete;

    bool Initialize(float planetRadius, int subdivisions = 4);
    void Shutdown();

    void Render(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        float planetRadius,
        const AtmosphereSettings& settings);

    bool IsReady() const { return _initialized; }

private:
    void GenerateAtmosphereSphere(float radius, int subdivisions);

    Mesh _atmosphereMesh;
    Shader _atmosphereShader;
    bool _initialized = false;
};

} // namespace planets::render::effects
