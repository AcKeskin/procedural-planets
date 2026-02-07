#pragma once

#include "QuadTreeNode.h"
#include "SpherePatch.h"
#include "PatchPool.h"
#include "../TerrainGenerator.h"
#include "../settings/TerrainSettings.h"
#include "../settings/SurfaceSettings.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace planets::render::lod {

// Configuration for planet-wide quadtree LOD
struct QuadTreeConfig
{
    float planetRadius      = 10.0f;
    int   baseSubdivisions  = 2;       // Icosahedron subdivisions for root patches
    int   meshResolution    = 32;      // Vertices per edge for all patches
    int   maxDepth          = 8;       // Maximum tree depth
    float splitThreshold    = 2.0f;    // Split when distance < threshold * arcLength * radius
    float hysteresis        = 1.3f;    // Merge threshold multiplier to prevent flickering
    int   maxActivePatches  = 400;     // Budget limit for active leaf patches
    float skirtFraction     = 0.02f;   // Skirt depth as fraction of patch arc length
};

// Top-level manager for adaptive planet LOD using quadtree hierarchy
// Owns root nodes from subdivided icosahedron and handles split/merge traversal
class PlanetQuadTree
{
public:
    PlanetQuadTree() = default;
    ~PlanetQuadTree() = default;

    PlanetQuadTree(const PlanetQuadTree&) = delete;
    PlanetQuadTree& operator=(const PlanetQuadTree&) = delete;

    // Initialize tree from icosahedron and generate initial patches
    void Initialize(
        const QuadTreeConfig& config,
        TerrainGenerator& terrainGen,
        const EarthTerrainSettings& terrainSettings,
        const EarthShadingSettings& shadingSettings,
        uint32_t seed);

    // Per-frame update: traverse tree and apply split/merge decisions
    void Update(const glm::vec3& cameraPos, const glm::mat4& viewProjection);

    // Render all visible leaf patches
    void Render(const Shader& shader) const;

    // Statistics for debug panel
    int GetActiveLeafCount() const { return _activeLeafCount; }
    int GetTotalNodeCount() const { return _totalNodeCount; }
    int GetVisiblePatchCount() const { return _visiblePatchCount; }

private:
    // Build root nodes from subdivided icosahedron
    void BuildRootNodes(int subdivisions);

    // Recursive traversal for split/merge decisions
    void TraverseAndUpdate(QuadTreeNode& node, const glm::vec3& cameraPos);

    // Generate patch data for a node
    void GeneratePatchForNode(QuadTreeNode& node);

    // Collect all leaf nodes for rendering
    void CollectLeaves(QuadTreeNode& node, std::vector<QuadTreeNode*>& leaves) const;

    std::vector<std::unique_ptr<QuadTreeNode>> _roots;
    QuadTreeConfig _config;
    PatchPool _patchPool;

    // Non-owning references set during Initialize
    TerrainGenerator* _terrainGen = nullptr;
    const EarthTerrainSettings* _terrainSettings = nullptr;
    const EarthShadingSettings* _shadingSettings = nullptr;
    uint32_t _seed = 0;

    // Per-frame state
    int _activeLeafCount = 0;
    int _totalNodeCount = 0;
    mutable int _visiblePatchCount = 0;
    Frustum _currentFrustum;
};

} // namespace planets::render::lod
