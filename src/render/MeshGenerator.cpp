#include "MeshGenerator.h"
#include "../core/generation/Planet.h"
#include "../core/math/Constants.h"
#include <cmath>
#include <map>

namespace planets::render
{

namespace
{
    constexpr float HeightNormalizationScale = 10.0f;
    constexpr float HeightNormalizationOffset = 0.5f;
}

MeshData MeshGenerator::GenerateIcosphere(int subdivisions)
{
    MeshData mesh;

    // Golden ratio
    const float t = core::GoldenRatio;

    // Create 12 vertices of icosahedron
    auto addVertex = [&mesh](float x, float y, float z) -> uint32_t
    {
        glm::vec3 pos = glm::normalize(glm::vec3(x, y, z));
        Vertex v;
        v.position = pos;
        v.normal = pos;
        v.uv = SphericalUV(pos);
        v.shadingData = glm::vec4(0.0f);
        mesh.vertices.push_back(v);
        return static_cast<uint32_t>(mesh.vertices.size() - 1);
    };

    addVertex(-1, t, 0);
    addVertex(1, t, 0);
    addVertex(-1, -t, 0);
    addVertex(1, -t, 0);

    addVertex(0, -1, t);
    addVertex(0, 1, t);
    addVertex(0, -1, -t);
    addVertex(0, 1, -t);

    addVertex(t, 0, -1);
    addVertex(t, 0, 1);
    addVertex(-t, 0, -1);
    addVertex(-t, 0, 1);

    // 20 faces of icosahedron
    mesh.indices = {0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4,  11, 10, 2,  10, 7, 6, 7, 1, 8,
                    3, 9,  4, 3, 4, 2, 3, 2, 6, 3, 6, 8,  3, 8,  9,  4, 9, 5, 2, 4,  11, 6,  2,  10, 8,  6, 7, 9, 8, 1};

    // Subdivide
    for (int i = 0; i < subdivisions; ++i)
    {
        Subdivide(mesh);
    }

    return mesh;
}

MeshData MeshGenerator::GeneratePlanetMesh(const planets::core::Planet& planet, int subdivisions)
{
    MeshData mesh = GenerateIcosphere(subdivisions);

    const auto& settings = planet.GetSettings();

    // Displace vertices along normals based on noise
    for (auto& vertex : mesh.vertices)
    {
        glm::vec3 dir = glm::normalize(vertex.position);
        float radius = planet.GetRadiusAt(dir.x, dir.y, dir.z);

        vertex.position = dir * radius;
        // Store normalized height in UV.x for coloring
        float height = planet.SampleHeight(dir.x, dir.y, dir.z);
        vertex.uv.x = height;
    }

    // Recalculate normals after displacement
    mesh.RecalculateNormals();

    return mesh;
}

MeshData MeshGenerator::GeneratePlanetMesh(int subdivisions, float baseRadius, const std::vector<float>& heights)
{
    MeshData mesh = GenerateIcosphere(subdivisions);

    if (heights.size() != mesh.vertices.size())
    {
        return mesh;
    }

    // Apply GPU-computed heights
    for (size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        glm::vec3 dir = glm::normalize(mesh.vertices[i].position);
        float radius = heights[i] * baseRadius;
        mesh.vertices[i].position = dir * radius;
        // Store height for coloring (normalized around 1.0)
        mesh.vertices[i].uv.x = (heights[i] - 1.0f) * HeightNormalizationScale + HeightNormalizationOffset;
        // shadingData already initialized to vec4(0) by GenerateIcosphere
    }

    mesh.RecalculateNormals();
    return mesh;
}

MeshData MeshGenerator::GeneratePlanetMesh(int subdivisions,
                                           float baseRadius,
                                           const std::vector<float>& heights,
                                           const std::vector<glm::vec4>& shadingData)
{
    MeshData mesh = GenerateIcosphere(subdivisions);

    if (heights.size() != mesh.vertices.size())
    {
        return mesh;
    }

    // Apply GPU-computed heights and shading data
    for (size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        glm::vec3 dir = glm::normalize(mesh.vertices[i].position);
        float radius = heights[i] * baseRadius;
        mesh.vertices[i].position = dir * radius;
        // Store height for coloring (normalized around 1.0)
        mesh.vertices[i].uv.x = (heights[i] - 1.0f) * HeightNormalizationScale + HeightNormalizationOffset;

        // Assign shading data if available
        if (i < shadingData.size())
        {
            mesh.vertices[i].shadingData = shadingData[i];
        }
        else
        {
            mesh.vertices[i].shadingData = glm::vec4(0.0f);
        }
    }

    mesh.RecalculateNormals();
    return mesh;
}

std::vector<glm::vec3> MeshGenerator::GetIcosphereVertices(int subdivisions)
{
    MeshData mesh = GenerateIcosphere(subdivisions);
    std::vector<glm::vec3> positions;
    positions.reserve(mesh.vertices.size());
    for (const auto& vertex : mesh.vertices)
    {
        positions.push_back(vertex.position);
    }
    return positions;
}

void MeshGenerator::Subdivide(MeshData& mesh)
{
    std::map<uint64_t, uint32_t> midpointCache;
    std::vector<uint32_t> newIndices;

    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        uint32_t v1 = mesh.indices[i];
        uint32_t v2 = mesh.indices[i + 1];
        uint32_t v3 = mesh.indices[i + 2];

        uint32_t a = GetMidpoint(mesh, midpointCache, v1, v2);
        uint32_t b = GetMidpoint(mesh, midpointCache, v2, v3);
        uint32_t c = GetMidpoint(mesh, midpointCache, v3, v1);

        newIndices.push_back(v1);
        newIndices.push_back(a);
        newIndices.push_back(c);
        newIndices.push_back(v2);
        newIndices.push_back(b);
        newIndices.push_back(a);
        newIndices.push_back(v3);
        newIndices.push_back(c);
        newIndices.push_back(b);
        newIndices.push_back(a);
        newIndices.push_back(b);
        newIndices.push_back(c);
    }

    mesh.indices = std::move(newIndices);
}

uint32_t MeshGenerator::GetMidpoint(MeshData& mesh, std::map<uint64_t, uint32_t>& cache, uint32_t i1, uint32_t i2)
{
    // Ensure consistent ordering
    if (i1 > i2)
    {
        std::swap(i1, i2);
    }

    uint64_t key = (static_cast<uint64_t>(i1) << 32) | i2;

    auto it = cache.find(key);
    if (it != cache.end())
    {
        return it->second;
    }

    const Vertex& v1 = mesh.vertices[i1];
    const Vertex& v2 = mesh.vertices[i2];

    glm::vec3 midPos = glm::normalize((v1.position + v2.position) * 0.5f);

    Vertex midVertex;
    midVertex.position = midPos;
    midVertex.normal = midPos;
    midVertex.uv = SphericalUV(midPos);
    midVertex.shadingData = glm::vec4(0.0f);

    uint32_t index = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(midVertex);
    cache[key] = index;

    return index;
}

glm::vec2 MeshGenerator::SphericalUV(const glm::vec3& position)
{
    float u = 0.5f + std::atan2(position.z, position.x) / core::TwoPi;
    float v = 0.5f - std::asin(position.y) / core::Pi;
    return glm::vec2(u, v);
}

} // namespace planets::render
