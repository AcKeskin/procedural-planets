#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>
#include "../settings/CloudSettings.h"

namespace planets::render::effects
{

// Renders clouds as a lit sphere-shell above the surface, shaded from an animated coverage-noise
// field (not raymarched). Drawn into the scene before the atmosphere pass so the limb scatters
// over the cloud tops.
class CloudRenderer
{
public:
    CloudRenderer();
    ~CloudRenderer();

    CloudRenderer(const CloudRenderer&) = delete;
    CloudRenderer& operator=(const CloudRenderer&) = delete;

    bool Initialize(float planetRadius);
    void Shutdown();

    void Render(const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& cameraPos,
                const glm::vec3& lightDir,
                const glm::vec3& sunColor,
                float sunIntensity,
                float ambientLight,
                float elapsedTime,
                const CloudSettings& settings);

    bool IsReady() const { return _initialized; }

private:
    void GenerateShell(float radius, int subdivisions);

    Mesh _cloudMesh;
    Shader _cloudShader;
    float _planetRadius = 1.0f;
    bool _initialized = false;
};

} // namespace planets::render::effects
