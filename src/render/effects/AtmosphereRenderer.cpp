#include "AtmosphereRenderer.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <array>
#include <map>
#include <iostream>

namespace planets::render::effects {

// Icosahedron base geometry
static const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;

static const glm::vec3 IcosahedronVertices[12] = {
    glm::normalize(glm::vec3(-1,  PHI,  0)),
    glm::normalize(glm::vec3( 1,  PHI,  0)),
    glm::normalize(glm::vec3(-1, -PHI,  0)),
    glm::normalize(glm::vec3( 1, -PHI,  0)),
    glm::normalize(glm::vec3( 0, -1,  PHI)),
    glm::normalize(glm::vec3( 0,  1,  PHI)),
    glm::normalize(glm::vec3( 0, -1, -PHI)),
    glm::normalize(glm::vec3( 0,  1, -PHI)),
    glm::normalize(glm::vec3( PHI,  0, -1)),
    glm::normalize(glm::vec3( PHI,  0,  1)),
    glm::normalize(glm::vec3(-PHI,  0, -1)),
    glm::normalize(glm::vec3(-PHI,  0,  1))
};

static const int IcosahedronFaces[20][3] = {
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
    {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
    {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
    {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
};

AtmosphereRenderer::AtmosphereRenderer() = default;

AtmosphereRenderer::~AtmosphereRenderer()
{
    Shutdown();
}

bool AtmosphereRenderer::Initialize(float planetRadius, int subdivisions)
{
    if (!_atmosphereShader.LoadFromFiles("shaders/atmosphere.vert", "shaders/atmosphere.frag"))
    {
        std::cerr << "[AtmosphereRenderer] Failed to load atmosphere shaders" << std::endl;
        return false;
    }

    // Generate unit sphere (will be scaled in render)
    GenerateAtmosphereSphere(1.0f, subdivisions);

    _initialized = true;
    std::cout << "[AtmosphereRenderer] Initialized" << std::endl;
    return true;
}

void AtmosphereRenderer::Shutdown()
{
    _initialized = false;
}

void AtmosphereRenderer::GenerateAtmosphereSphere(float radius, int subdivisions)
{
    MeshData meshData;

    using EdgeKey = std::pair<uint32_t, uint32_t>;
    std::map<EdgeKey, uint32_t> edgeCache;

    auto getMidpoint = [&](uint32_t i1, uint32_t i2) -> uint32_t {
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
        vert.uv = glm::vec2(0.0f);
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
        vert.uv = glm::vec2(0.0f);
        meshData.vertices.push_back(vert);
    }

    // Create initial faces
    std::vector<std::array<uint32_t, 3>> faces;
    for (const auto& f : IcosahedronFaces)
    {
        faces.push_back({static_cast<uint32_t>(f[0]),
                         static_cast<uint32_t>(f[1]),
                         static_cast<uint32_t>(f[2])});
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

    _atmosphereMesh.Upload(meshData);
}

void AtmosphereRenderer::Render(
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPos,
    const glm::vec3& lightDir,
    float planetRadius,
    const AtmosphereSettings& settings)
{
    if (!_initialized || !settings.enabled)
    {
        return;
    }

    // Enable additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Disable depth write so atmosphere doesn't occlude terrain
    glDepthMask(GL_FALSE);

    // Cull front faces so we render the inside of the sphere
    glCullFace(GL_FRONT);

    _atmosphereShader.Use();

    // Scale model to atmosphere size
    float atmosphereRadius = planetRadius * (1.0f + settings.height);
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(atmosphereRadius));

    _atmosphereShader.SetMat4("uModel", model);
    _atmosphereShader.SetMat4("uView", view);
    _atmosphereShader.SetMat4("uProjection", projection);
    _atmosphereShader.SetVec3("uCameraPos", cameraPos);
    _atmosphereShader.SetVec3("uLightDir", lightDir);
    _atmosphereShader.SetVec3("uAtmosphereColor", settings.color);
    _atmosphereShader.SetFloat("uDensityFalloff", settings.densityFalloff);
    _atmosphereShader.SetFloat("uIntensity", settings.intensity);

    _atmosphereMesh.Draw();

    // Restore state
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace planets::render::effects
