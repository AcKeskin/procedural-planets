#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>
#include "../settings/OceanSettings.h"

namespace planets::render::effects {

// Renders ocean as a sphere at sea level
class OceanRenderer
{
public:
    OceanRenderer();
    ~OceanRenderer();

    OceanRenderer(const OceanRenderer&) = delete;
    OceanRenderer& operator=(const OceanRenderer&) = delete;

    bool Initialize(float planetRadius, float seaLevel, int subdivisions = 5);
    void Shutdown();

    void Render(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        float elapsedTime,
        const OceanSettings& settings);

    bool IsReady() const { return _initialized; }

private:
    void GenerateOceanSphere(float radius, int subdivisions);

    Mesh _oceanMesh;
    Shader _oceanShader;
    bool _initialized = false;
};

} // namespace planets::render::effects
