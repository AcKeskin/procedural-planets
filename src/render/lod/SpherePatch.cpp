#include "SpherePatch.h"
#include "PatchBindings.h"
#include "SharedPatchTopology.h"
#include "../Shader.h"
#include <cmath>
#include <algorithm>

namespace planets::render::lod
{

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

void SpherePatch::AdoptBuffers(GpuBuffer<float>&& positionBuffer,
                              GpuBuffer<float>&& heightBuffer,
                              GpuBuffer<float>&& normalBuffer,
                              GpuBuffer<glm::vec4>&& shadingBuffer)
{
    _positionBuffer = std::move(positionBuffer);
    _heightBuffer = std::move(heightBuffer);
    _normalBuffer = std::move(normalBuffer);
    _shadingBuffer = std::move(shadingBuffer);

    // Conservative culling AABB from the unit-sphere grid extents scaled by radius, widened by a
    // height margin. Height is a multiplier ~1.0 (ocean depth below, peaks a few % above); the
    // margin over-covers so a displaced vertex never escapes the box. Over-large is safe for
    // culling — it can only draw an off-screen patch, never wrongly cull a visible one.
    constexpr float kHeightMargin = 0.2f;
    const float lo = _planetRadius * (1.0f - kHeightMargin);
    const float hi = _planetRadius * (1.0f + kHeightMargin);
    _boundingBox.min = glm::vec3(std::numeric_limits<float>::max());
    _boundingBox.max = glm::vec3(std::numeric_limits<float>::lowest());
    for (const glm::vec3& unit : _vertices)
    {
        _boundingBox.min = glm::min(_boundingBox.min, glm::min(unit * lo, unit * hi));
        _boundingBox.max = glm::max(_boundingBox.max, glm::max(unit * lo, unit * hi));
    }

    _drawable = true;
}

bool SpherePatch::IsVisible(const Frustum& frustum) const
{
    return frustum.Intersects(_boundingBox);
}

void SpherePatch::Render(const Shader& shader, const SharedPatchTopology& topology) const
{
    if (!_drawable)
        return;

    _positionBuffer.Bind(kPatchPositionBinding);
    _heightBuffer.Bind(kPatchHeightBinding);
    _normalBuffer.Bind(kPatchNormalBinding);
    _shadingBuffer.Bind(kPatchShadingBinding);

    // Skirt depth matches the old CPU AppendSkirtGeometry: fraction of patch arc length × radius.
    const float skirtDepth = _skirtFraction * _angularSize * _planetRadius;
    shader.SetInt("uResolution", _resolution);
    shader.SetFloat("uPlanetRadius", _planetRadius);
    shader.SetFloat("uSkirtDepth", skirtDepth);

    topology.Draw();
}

} // namespace planets::render::lod
