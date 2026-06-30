#include "SpherePatch.h"
#include <cmath>
#include <algorithm>

namespace planets::render::lod
{

using planets::render::MeshData;
using planets::render::Vertex;

bool AABB::Contains(const glm::vec3& point) const
{
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y && point.z >= min.z &&
           point.z <= max.z;
}

Frustum Frustum::FromViewProjection(const glm::mat4& vp)
{
    Frustum f;

    // Extract planes from view-projection matrix
    // Left plane
    f.planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);

    // Right plane
    f.planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);

    // Bottom plane
    f.planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);

    // Top plane
    f.planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);

    // Near plane
    f.planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);

    // Far plane
    f.planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

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
        if (plane.x >= 0)
            positive.x = box.max.x;
        if (plane.y >= 0)
            positive.y = box.max.y;
        if (plane.z >= 0)
            positive.z = box.max.z;

        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0)
        {
            return false;
        }
    }
    return true;
}

void SpherePatch::Initialize(const glm::vec3& v0,
                             const glm::vec3& v1,
                             const glm::vec3& v2,
                             float planetRadius,
                             int resolution,
                             float skirtFraction)
{
    _v0 = glm::normalize(v0);
    _v1 = glm::normalize(v1);
    _v2 = glm::normalize(v2);
    _planetRadius = planetRadius;
    _resolution = resolution;
    _skirtFraction = skirtFraction;

    // Calculate center
    _center = glm::normalize(_v0 + _v1 + _v2);

    // Calculate angular size (approximate as max angle from center to corner)
    float angle0 = std::acos(std::clamp(glm::dot(_center, _v0), -1.0f, 1.0f));
    float angle1 = std::acos(std::clamp(glm::dot(_center, _v1), -1.0f, 1.0f));
    float angle2 = std::acos(std::clamp(glm::dot(_center, _v2), -1.0f, 1.0f));
    _angularSize = std::max({angle0, angle1, angle2});

    // Pre-generate unit sphere vertices
    GenerateGridVertices(_resolution, _vertices);
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
            glm::vec3 pos = _v0 * (1.0f - u) * (1.0f - v) + _v1 * u * (1.0f - v) + _v2 * v;

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

void SpherePatch::GenerateMesh(const std::vector<float>& heights,
                               const std::vector<glm::vec4>& shadingData,
                               const std::vector<glm::vec3>& computedNormals)
{
    // Apply heights and scale to planet radius
    MeshData meshData;
    meshData.vertices.resize(_vertices.size());

    bool hasComputedNormals = computedNormals.size() == _vertices.size();

    // Geomorph target height per vertex: the height the patch would have at HALF resolution
    // (its parent LOD). Odd grid rows/cols sit between coarse samples, so their morph height is
    // the average of the even neighbours that survive decimation. Even/even points are unchanged.
    auto heightAt = [&](int i, int j) -> float {
        size_t idx = static_cast<size_t>(j) * _resolution + i;
        return idx < heights.size() ? heights[idx] : 1.0f;
    };
    auto morphHeightAt = [&](int i, int j) -> float {
        bool oddI = (i & 1) != 0;
        bool oddJ = (j & 1) != 0;
        int i0 = i - (oddI ? 1 : 0), i1 = i + (oddI ? 1 : 0);
        int j0 = j - (oddJ ? 1 : 0), j1 = j + (oddJ ? 1 : 0);
        i1 = std::min(i1, _resolution - 1);
        j1 = std::min(j1, _resolution - 1);
        if (oddI && oddJ) // diagonal: average the four even corners
            return 0.25f * (heightAt(i0, j0) + heightAt(i1, j0) + heightAt(i0, j1) + heightAt(i1, j1));
        if (oddI)
            return 0.5f * (heightAt(i0, j) + heightAt(i1, j));
        if (oddJ)
            return 0.5f * (heightAt(i, j0) + heightAt(i, j1));
        return heightAt(i, j);
    };

    for (size_t i = 0; i < _vertices.size(); ++i)
    {
        float h = (i < heights.size()) ? heights[i] : 1.0f;
        glm::vec3 pos = _vertices[i] * _planetRadius * h;

        int gx = static_cast<int>(i % _resolution);
        int gy = static_cast<int>(i / _resolution);
        float morphH = morphHeightAt(gx, gy);
        glm::vec3 morphPos = _vertices[i] * _planetRadius * morphH;

        meshData.vertices[i].position = pos;
        meshData.vertices[i].normal = hasComputedNormals ? computedNormals[i] : glm::vec3(0.0f);
        meshData.vertices[i].uv = glm::vec2(h, 0.0f); // Store height in UV.x for shader access
        meshData.vertices[i].shadingData = (i < shadingData.size()) ? shadingData[i] : glm::vec4(0.0f);
        meshData.vertices[i].morphPosition = morphPos;
    }

    // Generate indices
    GenerateGridIndices(_resolution, meshData.indices);

    // Only fall back to geometric normals if analytical normals were not provided
    if (!hasComputedNormals)
    {
        meshData.RecalculateNormals();
    }

    // Append skirt geometry to hide cracks between LOD levels
    AppendSkirtGeometry(meshData, heights, shadingData);

    // Update bounding box
    _boundingBox.min = glm::vec3(std::numeric_limits<float>::max());
    _boundingBox.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& vert : meshData.vertices)
    {
        _boundingBox.min = glm::min(_boundingBox.min, vert.position);
        _boundingBox.max = glm::max(_boundingBox.max, vert.position);
    }

    // Upload mesh
    _mesh.Upload(meshData);
    _generated = true;
}

bool SpherePatch::IsVisible(const Frustum& frustum) const
{
    return frustum.Intersects(_boundingBox);
}

void SpherePatch::Render() const
{
    if (_generated)
    {
        _mesh.Draw();
    }
}

void SpherePatch::AppendSkirtGeometry(MeshData& meshData,
                                      const std::vector<float>& heights,
                                      const std::vector<glm::vec4>& shadingData)
{
    // Skirt depth proportional to patch angular size
    const float skirtDepth = _skirtFraction * _angularSize * _planetRadius;
    const int res = _resolution;
    const uint32_t baseVertexCount = static_cast<uint32_t>(meshData.vertices.size());

    // Helper to add skirt edge
    auto AddSkirtEdge = [&](const std::vector<uint32_t>& edgeIndices)
    {
        const uint32_t skirtStartIndex = static_cast<uint32_t>(meshData.vertices.size());

        // Create skirt vertices by duplicating edge vertices and pushing them toward planet center
        for (uint32_t edgeIdx : edgeIndices)
        {
            const Vertex& originalVert = meshData.vertices[edgeIdx];
            Vertex skirtVert = originalVert;

            // Offset toward planet center
            glm::vec3 radialDir = glm::normalize(originalVert.position);
            skirtVert.position = originalVert.position - radialDir * skirtDepth;
            // Morph target offset the same way so skirts collapse with the surface, not independently
            glm::vec3 morphRadial = glm::normalize(originalVert.morphPosition);
            skirtVert.morphPosition = originalVert.morphPosition - morphRadial * skirtDepth;

            meshData.vertices.push_back(skirtVert);
        }

        // Create triangles connecting edge to skirt
        for (size_t i = 0; i < edgeIndices.size() - 1; ++i)
        {
            uint32_t edge0 = edgeIndices[i];
            uint32_t edge1 = edgeIndices[i + 1];
            uint32_t skirt0 = skirtStartIndex + static_cast<uint32_t>(i);
            uint32_t skirt1 = skirt0 + 1;

            // Two triangles per quad segment
            meshData.indices.push_back(edge0);
            meshData.indices.push_back(skirt0);
            meshData.indices.push_back(edge1);

            meshData.indices.push_back(edge1);
            meshData.indices.push_back(skirt0);
            meshData.indices.push_back(skirt1);
        }
    };

    // Bottom edge: row 0
    std::vector<uint32_t> bottomEdge;
    bottomEdge.reserve(res);
    for (int i = 0; i < res; ++i)
    {
        bottomEdge.push_back(i);
    }
    AddSkirtEdge(bottomEdge);

    // Top edge: row (res-1)
    std::vector<uint32_t> topEdge;
    topEdge.reserve(res);
    for (int i = 0; i < res; ++i)
    {
        topEdge.push_back((res - 1) * res + i);
    }
    AddSkirtEdge(topEdge);

    // Left edge: column 0
    std::vector<uint32_t> leftEdge;
    leftEdge.reserve(res);
    for (int j = 0; j < res; ++j)
    {
        leftEdge.push_back(j * res);
    }
    AddSkirtEdge(leftEdge);

    // Right edge: column (res-1)
    std::vector<uint32_t> rightEdge;
    rightEdge.reserve(res);
    for (int j = 0; j < res; ++j)
    {
        rightEdge.push_back(j * res + (res - 1));
    }
    AddSkirtEdge(rightEdge);
}

} // namespace planets::render::lod
