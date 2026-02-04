#pragma once

#include "../Mesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace planets::render::lod {

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

// Single terrain patch with multiple LOD levels
class SpherePatch
{
public:
    static constexpr int MaxLodLevels = 4;

    SpherePatch() = default;
    ~SpherePatch() = default;

    SpherePatch(const SpherePatch&) = delete;
    SpherePatch& operator=(const SpherePatch&) = delete;

    SpherePatch(SpherePatch&&) noexcept = default;
    SpherePatch& operator=(SpherePatch&&) noexcept = default;

    // Initialize patch from three vertices of base icosahedron face
    void Initialize(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float planetRadius);

    // Generate mesh data for a specific LOD level
    void GenerateLod(int lod, const std::vector<float>& heights);

    // Upload all LOD meshes to GPU
    void Upload();

    // Select appropriate LOD based on camera distance
    int SelectLod(const glm::vec3& cameraPos) const;

    // Check if patch is visible in frustum
    bool IsVisible(const Frustum& frustum) const;

    // Render the patch at specified LOD
    void Render(int lod) const;

    // Accessors
    const glm::vec3& GetCenter() const { return _center; }
    float GetAngularSize() const { return _angularSize; }
    const AABB& GetBoundingBox() const { return _boundingBox; }

    // Get vertices for height generation (unit sphere positions)
    const std::vector<glm::vec3>& GetVertices(int lod) const { return _lodVertices[lod]; }
    int GetVertexCount(int lod) const;

private:
    void GenerateGridVertices(int resolution, std::vector<glm::vec3>& vertices);
    void GenerateGridIndices(int resolution, std::vector<uint32_t>& indices);
    void AddSkirt(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, int resolution);

    glm::vec3 _v0, _v1, _v2;        // Base triangle vertices (unit sphere)
    glm::vec3 _center;              // Patch center on unit sphere
    float _angularSize = 0.0f;      // Angular extent in radians
    float _planetRadius = 1.0f;
    AABB _boundingBox;

    std::array<std::vector<glm::vec3>, MaxLodLevels> _lodVertices;
    std::array<Mesh, MaxLodLevels> _lodMeshes;
    std::array<bool, MaxLodLevels> _lodGenerated = {};
};

} // namespace planets::render::lod
