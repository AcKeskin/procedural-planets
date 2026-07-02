#pragma once

#include "../GpuBuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace planets::render
{
class Shader;
}

namespace planets::render::lod
{

class SharedPatchTopology;

// Axis-aligned bounding box for frustum culling
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    bool Contains(const glm::vec3& point) const;
    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
};

// View frustum for culling
struct Frustum
{
    std::array<glm::vec4, 6> planes; // left, right, bottom, top, near, far

    static Frustum FromViewProjection(const glm::mat4& vp);
    bool Intersects(const AABB& box) const;
};

// Single terrain patch at fixed resolution
class SpherePatch
{
public:
    SpherePatch() = default;
    ~SpherePatch() = default;

    SpherePatch(const SpherePatch&) = delete;
    SpherePatch& operator=(const SpherePatch&) = delete;

    SpherePatch(SpherePatch&&) noexcept = default;
    SpherePatch& operator=(SpherePatch&&) noexcept = default;

    // Initialize patch from three vertices of base icosahedron face
    void Initialize(const glm::vec3& v0,
                    const glm::vec3& v1,
                    const glm::vec3& v2,
                    float planetRadius,
                    int resolution = 32,
                    float skirtFraction = 0.02f);

    // Take ownership of the compute-written GPU buffers (GPU-resident render path). The buffers
    // outlive the generation task and live as long as the patch; the draw binds them directly,
    // no readback. The culling AABB is derived conservatively from the unit-sphere grid extents
    // × a height margin (no CPU height copy — the data stays on the GPU). Marks the patch drawable.
    void AdoptBuffers(GpuBuffer<float>&& positionBuffer,
                      GpuBuffer<float>&& heightBuffer,
                      GpuBuffer<float>&& normalBuffer,
                      GpuBuffer<glm::vec4>&& shadingBuffer);

    bool IsDrawable() const { return _drawable; }

    // Check if patch is visible in frustum
    bool IsVisible(const Frustum& frustum) const;

    // Render the patch: bind the compute-written SSBOs, set per-patch uniforms, draw over the
    // shared topology. uMorphFactor is set by the caller (LOD state, not patch-owned).
    void Render(const Shader& shader, const SharedPatchTopology& topology) const;

    // Accessors
    const glm::vec3& GetCenter() const { return _center; }
    float GetAngularSize() const { return _angularSize; }
    const AABB& GetBoundingBox() const { return _boundingBox; }

    // Get vertices for height generation (unit sphere positions)
    const std::vector<glm::vec3>& GetUnitSphereVertices() const { return _vertices; }

private:
    void GenerateGridVertices(int resolution, std::vector<glm::vec3>& vertices);

    glm::vec3 _v0, _v1, _v2;   // Base triangle vertices (unit sphere)
    glm::vec3 _center;         // Patch center on unit sphere
    float _angularSize = 0.0f; // Angular extent in radians
    float _planetRadius = 1.0f;
    int _resolution = 32;         // Vertices per edge
    float _skirtFraction = 0.02f; // Fraction of patch size for skirt depth
    AABB _boundingBox;

    std::vector<glm::vec3> _vertices; // Unit sphere positions

    // GPU-resident render path: the compute-written buffers, owned for the patch's lifetime.
    GpuBuffer<float> _positionBuffer;
    GpuBuffer<float> _heightBuffer;
    GpuBuffer<float> _normalBuffer;
    GpuBuffer<glm::vec4> _shadingBuffer;
    bool _drawable = false;
};

} // namespace planets::render::lod
