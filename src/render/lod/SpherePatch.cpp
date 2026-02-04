#include "SpherePatch.h"
#include <cmath>
#include <algorithm>

namespace planets::render::lod {

using planets::render::MeshData;
using planets::render::Vertex;

// Resolution per LOD level (vertices per edge)
static constexpr int LodResolutions[SpherePatch::MaxLodLevels] = { 64, 32, 16, 8 };

// Distance thresholds for LOD selection (in planet radii)
static constexpr float LodThresholds[SpherePatch::MaxLodLevels] = { 2.0f, 4.0f, 8.0f, 16.0f };

bool AABB::Contains(const glm::vec3& point) const
{
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}

Frustum Frustum::FromViewProjection(const glm::mat4& vp)
{
    Frustum f;

    // Extract planes from view-projection matrix
    // Left plane
    f.planes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]);

    // Right plane
    f.planes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]);

    // Bottom plane
    f.planes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]);

    // Top plane
    f.planes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]);

    // Near plane
    f.planes[4] = glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]);

    // Far plane
    f.planes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]);

    // Normalize planes
    for (auto& plane : f.planes)
    {
        float len = glm::length(glm::vec3(plane));
        if (len > 0.0f)
        {
            plane /= len;
        }
    }

    return f;
}

bool Frustum::Intersects(const AABB& box) const
{
    for (const auto& plane : planes)
    {
        glm::vec3 positive = box.min;
        if (plane.x >= 0) positive.x = box.max.x;
        if (plane.y >= 0) positive.y = box.max.y;
        if (plane.z >= 0) positive.z = box.max.z;

        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0)
        {
            return false;
        }
    }
    return true;
}

void SpherePatch::Initialize(
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float planetRadius)
{
    _v0 = glm::normalize(v0);
    _v1 = glm::normalize(v1);
    _v2 = glm::normalize(v2);
    _planetRadius = planetRadius;

    // Calculate center
    _center = glm::normalize(_v0 + _v1 + _v2);

    // Calculate angular size (approximate as max angle from center to corner)
    float angle0 = std::acos(std::clamp(glm::dot(_center, _v0), -1.0f, 1.0f));
    float angle1 = std::acos(std::clamp(glm::dot(_center, _v1), -1.0f, 1.0f));
    float angle2 = std::acos(std::clamp(glm::dot(_center, _v2), -1.0f, 1.0f));
    _angularSize = std::max({ angle0, angle1, angle2 });

    // Pre-generate vertices for all LOD levels
    for (int lod = 0; lod < MaxLodLevels; ++lod)
    {
        GenerateGridVertices(LodResolutions[lod], _lodVertices[lod]);
    }
}

void SpherePatch::GenerateGridVertices(int resolution, std::vector<glm::vec3>& vertices)
{
    vertices.clear();
    vertices.reserve(resolution * resolution);

    // Generate grid of vertices on the triangle using barycentric interpolation
    for (int j = 0; j < resolution; ++j)
    {
        for (int i = 0; i < resolution; ++i)
        {
            // Barycentric coordinates within the triangle
            float u = static_cast<float>(i) / (resolution - 1);
            float v = static_cast<float>(j) / (resolution - 1);

            // Clamp to stay within triangle (u + v <= 1)
            // We use a square grid mapped to the triangle
            glm::vec3 pos = _v0 * (1.0f - u) * (1.0f - v) +
                           _v1 * u * (1.0f - v) +
                           _v2 * v;

            // Project onto unit sphere
            vertices.push_back(glm::normalize(pos));
        }
    }
}

void SpherePatch::GenerateGridIndices(int resolution, std::vector<uint32_t>& indices)
{
    indices.clear();
    indices.reserve((resolution - 1) * (resolution - 1) * 6);

    for (int j = 0; j < resolution - 1; ++j)
    {
        for (int i = 0; i < resolution - 1; ++i)
        {
            uint32_t topLeft = j * resolution + i;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (j + 1) * resolution + i;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

void SpherePatch::GenerateLod(int lod, const std::vector<float>& heights)
{
    if (lod < 0 || lod >= MaxLodLevels) return;

    const auto& unitVertices = _lodVertices[lod];
    int resolution = LodResolutions[lod];

    // Apply heights and scale to planet radius
    MeshData meshData;
    meshData.vertices.resize(unitVertices.size());

    for (size_t i = 0; i < unitVertices.size(); ++i)
    {
        float h = (i < heights.size()) ? heights[i] : 1.0f;
        glm::vec3 pos = unitVertices[i] * _planetRadius * h;

        // Calculate UV from spherical coordinates
        glm::vec3 n = glm::normalize(pos);
        float u = 0.5f + std::atan2(n.z, n.x) / (2.0f * 3.14159265f);
        float v = 0.5f - std::asin(std::clamp(n.y, -1.0f, 1.0f)) / 3.14159265f;

        meshData.vertices[i].position = pos;
        meshData.vertices[i].normal = glm::vec3(0.0f); // Will be calculated below
        meshData.vertices[i].uv = glm::vec2(u, v);
    }

    // Generate indices
    GenerateGridIndices(resolution, meshData.indices);

    // Calculate normals using MeshData's built-in method
    meshData.RecalculateNormals();

    // Update bounding box
    _boundingBox.min = glm::vec3(std::numeric_limits<float>::max());
    _boundingBox.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& vert : meshData.vertices)
    {
        _boundingBox.min = glm::min(_boundingBox.min, vert.position);
        _boundingBox.max = glm::max(_boundingBox.max, vert.position);
    }

    // Upload mesh
    _lodMeshes[lod].Upload(meshData);
    _lodGenerated[lod] = true;
}

void SpherePatch::Upload()
{
    // Meshes are uploaded in GenerateLod
}

int SpherePatch::SelectLod(const glm::vec3& cameraPos) const
{
    float dist = glm::length(cameraPos - _center * _planetRadius);
    float distInRadii = dist / _planetRadius;

    for (int lod = 0; lod < MaxLodLevels; ++lod)
    {
        if (distInRadii < LodThresholds[lod])
        {
            return lod;
        }
    }

    return MaxLodLevels - 1;
}

bool SpherePatch::IsVisible(const Frustum& frustum) const
{
    return frustum.Intersects(_boundingBox);
}

void SpherePatch::Render(int lod) const
{
    if (lod >= 0 && lod < MaxLodLevels && _lodGenerated[lod])
    {
        _lodMeshes[lod].Draw();
    }
}

int SpherePatch::GetVertexCount(int lod) const
{
    if (lod >= 0 && lod < MaxLodLevels)
    {
        return LodResolutions[lod] * LodResolutions[lod];
    }
    return 0;
}

} // namespace planets::render::lod
