#include "OceanRenderer.h"
#include "../../core/math/Constants.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <cmath>
#include <array>
#include <map>
#include <iostream>

namespace planets::render::effects
{

// Icosahedron base geometry
static const float PHI = planets::core::GoldenRatio;

static const glm::vec3 IcosahedronVertices[12] = {glm::normalize(glm::vec3(-1, PHI, 0)),
                                                  glm::normalize(glm::vec3(1, PHI, 0)),
                                                  glm::normalize(glm::vec3(-1, -PHI, 0)),
                                                  glm::normalize(glm::vec3(1, -PHI, 0)),
                                                  glm::normalize(glm::vec3(0, -1, PHI)),
                                                  glm::normalize(glm::vec3(0, 1, PHI)),
                                                  glm::normalize(glm::vec3(0, -1, -PHI)),
                                                  glm::normalize(glm::vec3(0, 1, -PHI)),
                                                  glm::normalize(glm::vec3(PHI, 0, -1)),
                                                  glm::normalize(glm::vec3(PHI, 0, 1)),
                                                  glm::normalize(glm::vec3(-PHI, 0, -1)),
                                                  glm::normalize(glm::vec3(-PHI, 0, 1))};

static const int IcosahedronFaces[20][3] = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                            {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                            {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                            {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};

OceanRenderer::OceanRenderer() = default;

OceanRenderer::~OceanRenderer()
{
    Shutdown();
}

bool OceanRenderer::Initialize(float planetRadius, float seaLevel, int subdivisions)
{
    if (!_oceanShader.LoadFromFiles("shaders/ocean.vert", "shaders/ocean.frag"))
    {
        std::cerr << "[OceanRenderer] Failed to load ocean shaders" << std::endl;
        return false;
    }

    float oceanRadius = planetRadius * seaLevel;
    GenerateOceanSphere(oceanRadius, subdivisions);

    _initialized = true;
    std::cout << "[OceanRenderer] Initialized with radius " << oceanRadius << std::endl;
    return true;
}

void OceanRenderer::Shutdown()
{
    _initialized = false;
}

void OceanRenderer::GenerateOceanSphere(float radius, int subdivisions)
{
    MeshData meshData;

    // Edge midpoint cache for subdivision
    using EdgeKey = std::pair<uint32_t, uint32_t>;
    std::map<EdgeKey, uint32_t> edgeCache;

    auto getMidpoint = [&](uint32_t i1, uint32_t i2) -> uint32_t
    {
        EdgeKey key = (i1 < i2) ? EdgeKey{i1, i2} : EdgeKey{i2, i1};
        auto it = edgeCache.find(key);
        if (it != edgeCache.end())
        {
            return it->second;
        }

        const auto& v1 = meshData.vertices[i1].position;
        const auto& v2 = meshData.vertices[i2].position;
        glm::vec3 mid = glm::normalize((v1 + v2) * 0.5f) * radius;

        uint32_t idx = static_cast<uint32_t>(meshData.vertices.size());
        Vertex vert;
        vert.position = mid;
        vert.normal = glm::normalize(mid);
        vert.uv = glm::vec2(0.5f + std::atan2(mid.z, mid.x) / planets::core::TwoPi,
                            0.5f - std::asin(glm::clamp(mid.y / radius, -1.0f, 1.0f)) / planets::core::Pi);
        meshData.vertices.push_back(vert);

        edgeCache[key] = idx;
        return idx;
    };

    // Create initial icosahedron vertices
    for (const auto& v : IcosahedronVertices)
    {
        glm::vec3 pos = v * radius;
        Vertex vert;
        vert.position = pos;
        vert.normal = v;
        vert.uv = glm::vec2(0.5f + std::atan2(pos.z, pos.x) / planets::core::TwoPi,
                            0.5f - std::asin(glm::clamp(v.y, -1.0f, 1.0f)) / planets::core::Pi);
        meshData.vertices.push_back(vert);
    }

    // Create initial faces
    std::vector<std::array<uint32_t, 3>> faces;
    for (const auto& f : IcosahedronFaces)
    {
        faces.push_back({static_cast<uint32_t>(f[0]), static_cast<uint32_t>(f[1]), static_cast<uint32_t>(f[2])});
    }

    // Subdivide
    for (int s = 0; s < subdivisions; ++s)
    {
        std::vector<std::array<uint32_t, 3>> newFaces;
        edgeCache.clear();

        for (const auto& face : faces)
        {
            uint32_t a = getMidpoint(face[0], face[1]);
            uint32_t b = getMidpoint(face[1], face[2]);
            uint32_t c = getMidpoint(face[2], face[0]);

            newFaces.push_back({face[0], a, c});
            newFaces.push_back({a, face[1], b});
            newFaces.push_back({c, b, face[2]});
            newFaces.push_back({a, b, c});
        }

        faces = std::move(newFaces);
    }

    // Convert faces to indices
    for (const auto& face : faces)
    {
        meshData.indices.push_back(face[0]);
        meshData.indices.push_back(face[1]);
        meshData.indices.push_back(face[2]);
    }

    _oceanMesh.Upload(meshData);

    std::cout << "[OceanRenderer] Generated ocean sphere: " << meshData.vertices.size() << " vertices, "
              << meshData.indices.size() / 3 << " triangles" << std::endl;
}

void OceanRenderer::Render(const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& cameraPos,
                           const glm::vec3& lightDir,
                           float elapsedTime,
                           const OceanSettings& settings)
{
    if (!_initialized || !settings.enabled)
    {
        return;
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _oceanShader.Use();

    glm::mat4 model = glm::mat4(1.0f);
    _oceanShader.SetMat4("uModel", model);
    _oceanShader.SetMat4("uView", view);
    _oceanShader.SetMat4("uProjection", projection);
    _oceanShader.SetVec3("uCameraPos", cameraPos);
    _oceanShader.SetVec3("uLightDir", lightDir);
    _oceanShader.SetVec3("uDeepColor", settings.deepColor);
    _oceanShader.SetVec3("uShallowColor", settings.shallowColor);
    _oceanShader.SetFloat("uFresnelPower", settings.fresnelPower);
    _oceanShader.SetFloat("uDepthMultiplier", settings.depthMultiplier);
    _oceanShader.SetFloat("uAlphaMultiplier", settings.alphaMultiplier);
    _oceanShader.SetFloat("uSmoothness", settings.smoothness);
    _oceanShader.SetFloat("uWaveStrength", settings.waveStrength);
    _oceanShader.SetFloat("uWaveScale", settings.waveScale);
    _oceanShader.SetFloat("uWaveSpeed", settings.waveSpeed);
    _oceanShader.SetFloat("uTime", elapsedTime);

    _oceanMesh.Draw();

    glDisable(GL_BLEND);
}

} // namespace planets::render::effects
