#pragma once

#include "QuadTreeNode.h"
#include "SpherePatch.h"
#include "PatchPool.h"
#include "../GenerationScheduler.h"
#include "../settings/TerrainSettings.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <memory>
#include <cstdint>

namespace planets::render
{
class Shader;
}

namespace planets::render::lod
{

// Configuration for planet-wide quadtree LOD
struct QuadTreeConfig
{
    float planetRadius = 10.0f;
    int baseSubdivisions = 2;    // Icosahedron subdivisions for root patches
    int meshResolution = 32;     // Vertices per edge for all patches
    int maxDepth = 8;            // Maximum tree depth
    float splitThreshold = 2.0f; // Split when distance < threshold * arcLength * radius
    float hysteresis = 1.3f;     // Merge threshold multiplier to prevent flickering
    int maxActivePatches = 400;  // Budget limit for active leaf patches
    float skirtFraction = 0.02f; // Skirt depth as fraction of patch arc length
};

// Adaptive planet LOD using quadtree hierarchy with async patch generation.
// Enqueues generation requests to a scheduler instead of generating synchronously.
// Old patches stay visible until replacements are ready.
class PlanetQuadTree
{
public:
    PlanetQuadTree() = default;
    ~PlanetQuadTree() = default;

    PlanetQuadTree(const PlanetQuadTree&) = delete;
    PlanetQuadTree& operator=(const PlanetQuadTree&) = delete;

    // Build tree structure and enqueue initial patches (non-blocking)
    void Initialize(const QuadTreeConfig& config, GenerationScheduler& scheduler, uint32_t seed);

    // Apply completed patches from scheduler into the tree
    void ApplyCompletedPatches();

    // Per-frame update: traverse tree and enqueue split/merge requests
    void Update(const glm::vec3& cameraPos, const glm::mat4& viewProjection);

    // Render visible patches, falling back to parent LOD when children aren't ready
    void Render(const Shader& shader) const;

    int GetActiveLeafCount() const { return _activeLeafCount; }
    int GetTotalNodeCount() const { return _totalNodeCount; }
    int GetVisiblePatchCount() const { return _visiblePatchCount; }

private:
    void BuildRootNodes(int subdivisions);
    void TraverseAndUpdate(QuadTreeNode& node, const glm::vec3& cameraPos);
    void EnqueueGeneration(QuadTreeNode& node, GenerationType type);
    bool IsPending(const QuadTreeNode* node) const;

    // Collect nodes to render, falling back to parent when children lack patches
    void CollectRenderableNodes(const QuadTreeNode& node, std::vector<const QuadTreeNode*>& nodes) const;

    // Check if every leaf in subtree has a patch
    bool IsSubtreeReady(const QuadTreeNode& node) const;

    // LOD distance thresholds for a node — single source of truth for split/merge/morph.
    float SplitDistance(const QuadTreeNode& node) const;
    float MergeDistance(const QuadTreeNode& node) const;

    std::vector<std::unique_ptr<QuadTreeNode>> _roots;
    QuadTreeConfig _config;
    PatchPool _patchPool;

    GenerationScheduler* _scheduler = nullptr;
    uint32_t _seed = 0;

    // Nodes with pending generation (to avoid duplicate requests)
    std::unordered_set<const QuadTreeNode*> _pendingNodes;

    glm::vec3 _lastCameraPos = glm::vec3(0.0f);
    int _activeLeafCount = 0;
    int _totalNodeCount = 0;
    mutable int _visiblePatchCount = 0;
    Frustum _currentFrustum;
};

} // namespace planets::render::lod
