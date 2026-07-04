#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>
#include "../settings/OceanSettings.h"
#include <planetgen/planetgen.h>

namespace planets::render::effects
{

// Renders ocean as a sphere at sea level
class OceanRenderer
{
public:
    OceanRenderer();
    ~OceanRenderer();

    OceanRenderer(const OceanRenderer&) = delete;
    OceanRenderer& operator=(const OceanRenderer&) = delete;

    // body + seed bake a real per-vertex bathymetry signal (sea level minus terrain height) into
    // the ocean mesh, so depth color + shoreline foam track the actual coast, not the view angle.
    bool Initialize(float planetRadius, float seaLevel, int subdivisions, PgBody body, uint32_t seed);
    void Shutdown();

    void Render(const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& cameraPos,
                const glm::vec3& lightDir,
                const glm::vec3& sunColor,
                float sunIntensity,
                float ambientLight,
                float elapsedTime,
                const OceanSettings& settings);

    bool IsReady() const { return _initialized; }

private:
    void GenerateOceanSphere(float radius, int subdivisions, PgBody body, float seaLevel, uint32_t seed);
    // Query terrain height per ocean vertex and store depth01 (sea level minus height, normalized)
    // in the vertex's uv.x — the real bathymetry signal the shader reads for depth color + foam.
    void BakeBathymetry(MeshData& mesh, PgBody body, float seaLevel, uint32_t seed);

    Mesh _oceanMesh;
    Shader _oceanShader;
    bool _initialized = false;
};

} // namespace planets::render::effects
