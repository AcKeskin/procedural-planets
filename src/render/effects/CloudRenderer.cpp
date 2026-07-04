#include "CloudRenderer.h"
#include "../../core/math/Constants.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <array>
#include <map>
#include <vector>
#include <cmath>
#include <iostream>

namespace planets::render::effects
{

namespace
{
constexpr int ShellSubdivisions = 5;

const float PHI = planets::core::GoldenRatio;

const glm::vec3 IcosahedronVertices[12] = {glm::normalize(glm::vec3(-1, PHI, 0)),
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

const int IcosahedronFaces[20][3] = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                     {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                     {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                     {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
} // namespace

CloudRenderer::CloudRenderer() = default;

CloudRenderer::~CloudRenderer()
{
    Shutdown();
}

bool CloudRenderer::Initialize(float planetRadius)
{
    if (!_cloudShader.LoadFromFiles("shaders/clouds.vert", "shaders/clouds.frag"))
    {
        std::cerr << "[CloudRenderer] Failed to load cloud shaders" << std::endl;
        return false;
    }

    _planetRadius = planetRadius;
    // Unit shell; the actual altitude is applied per-frame in the vertex shader so an altitude
    // tweak doesn't need a mesh rebuild.
    GenerateShell(1.0f, ShellSubdivisions);

    _initialized = true;
    std::cout << "[CloudRenderer] Initialized" << std::endl;
    return true;
}

void CloudRenderer::Shutdown()
{
    _initialized = false;
}

void CloudRenderer::GenerateShell(float radius, int subdivisions)
{
    MeshData meshData;

    using EdgeKey = std::pair<uint32_t, uint32_t>;
    std::map<EdgeKey, uint32_t> edgeCache;

    auto getMidpoint = [&](uint32_t i1, uint32_t i2) -> uint32_t
    {
        EdgeKey key = (i1 < i2) ? EdgeKey{i1, i2} : EdgeKey{i2, i1};
        auto it = edgeCache.find(key);
        if (it != edgeCache.end())
            return it->second;

        const auto& v1 = meshData.vertices[i1].position;
        const auto& v2 = meshData.vertices[i2].position;
        glm::vec3 mid = glm::normalize((v1 + v2) * 0.5f) * radius;

        uint32_t idx = static_cast<uint32_t>(meshData.vertices.size());
        Vertex vert;
        vert.position = mid;
        vert.normal = glm::normalize(mid);
        meshData.vertices.push_back(vert);
        edgeCache[key] = idx;
        return idx;
    };

    for (const auto& v : IcosahedronVertices)
    {
        Vertex vert;
        vert.position = v * radius;
        vert.normal = v;
        meshData.vertices.push_back(vert);
    }

    std::vector<std::array<uint32_t, 3>> faces;
    for (const auto& f : IcosahedronFaces)
        faces.push_back({static_cast<uint32_t>(f[0]), static_cast<uint32_t>(f[1]), static_cast<uint32_t>(f[2])});

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

    for (const auto& face : faces)
    {
        meshData.indices.push_back(face[0]);
        meshData.indices.push_back(face[1]);
        meshData.indices.push_back(face[2]);
    }

    _cloudMesh.Upload(meshData);
    std::cout << "[CloudRenderer] Generated cloud shell: " << meshData.vertices.size() << " vertices" << std::endl;
}

void CloudRenderer::Render(const glm::mat4& view,
                           const glm::mat4& projection,
                           const glm::vec3& cameraPos,
                           const glm::vec3& lightDir,
                           const glm::vec3& sunColor,
                           float sunIntensity,
                           float ambientLight,
                           float elapsedTime,
                           const CloudSettings& settings)
{
    if (!_initialized || !settings.enabled)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // The shell is drawn over the surface; keep depth test so terrain in front occludes it, but
    // don't write depth (a translucent layer shouldn't block the atmosphere's depth reconstruction).
    glDepthMask(GL_FALSE);

    _cloudShader.Use();

    _cloudShader.SetMat4("uView", view);
    _cloudShader.SetMat4("uProjection", projection);
    _cloudShader.SetVec3("uCameraPos", cameraPos);
    _cloudShader.SetVec3("uLightDir", lightDir);
    _cloudShader.SetVec3("uSunColor", sunColor);
    _cloudShader.SetFloat("uSunIntensity", sunIntensity);
    _cloudShader.SetFloat("uAmbientLight", ambientLight);
    _cloudShader.SetFloat("uShellRadius", _planetRadius * (1.0f + settings.altitude));
    _cloudShader.SetVec3("uColor", settings.color);
    _cloudShader.SetFloat("uCoverage", settings.coverage);
    _cloudShader.SetFloat("uDensity", settings.density);
    _cloudShader.SetFloat("uScale", settings.scale);
    _cloudShader.SetFloat("uSpeed", settings.speed);
    _cloudShader.SetFloat("uTime", elapsedTime);

    _cloudMesh.Draw();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace planets::render::effects
