#include "PlanetQuadTree.h"
#include "../Shader.h"
#include <iostream>
#include <cmath>

namespace planets::render::lod {

// Icosahedron vertices (golden ratio based)
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

// Icosahedron faces (20 triangles)
static const int IcosahedronFaces[20][3] = {
    // 5 faces around point 0
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
    // 5 adjacent faces
    {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
    // 5 faces around point 3
    {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
    // 5 adjacent faces
    {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
};

void PlanetQuadTree::Initialize(
    const QuadTreeConfig& config,
    GenerationScheduler& scheduler,
    uint32_t seed)
{
    _config = config;
    _scheduler = &scheduler;
    _seed = seed;

    _patchPool.Clear();
    _pendingNodes.clear();
    _roots.clear();

    BuildRootNodes(_config.baseSubdivisions);

    std::cout << "[PlanetQuadTree] Created " << _roots.size() << " root nodes" << std::endl;

    // Enqueue all root patches for async generation (non-blocking)
    for (auto& root : _roots)
    {
        EnqueueGeneration(*root, GenerationType::Initial);
    }

    std::cout << "[PlanetQuadTree] Enqueued " << _roots.size() << " initial patches" << std::endl;
}

void PlanetQuadTree::BuildRootNodes(int subdivisions)
{
    struct Face { glm::vec3 v0, v1, v2; };
    std::vector<Face> faces;

    for (const auto& f : IcosahedronFaces)
    {
        faces.push_back({
            IcosahedronVertices[f[0]],
            IcosahedronVertices[f[1]],
            IcosahedronVertices[f[2]]
        });
    }

    for (int s = 0; s < subdivisions; ++s)
    {
        std::vector<Face> newFaces;
        newFaces.reserve(faces.size() * 4);

        for (const auto& face : faces)
        {
            glm::vec3 m01 = glm::normalize((face.v0 + face.v1) * 0.5f);
            glm::vec3 m12 = glm::normalize((face.v1 + face.v2) * 0.5f);
            glm::vec3 m20 = glm::normalize((face.v2 + face.v0) * 0.5f);

            newFaces.push_back({ face.v0, m01, m20 });
            newFaces.push_back({ m01, face.v1, m12 });
            newFaces.push_back({ m20, m12, face.v2 });
            newFaces.push_back({ m01, m12, m20 });
        }

        faces = std::move(newFaces);
    }

    _roots.reserve(faces.size());
    for (const auto& face : faces)
    {
        _roots.push_back(std::make_unique<QuadTreeNode>(
            face.v0, face.v1, face.v2, 0, nullptr));
    }
}

void PlanetQuadTree::ApplyCompletedPatches()
{
    if (!_scheduler)
        return;

    auto completed = _scheduler->TakeCompleted();
    for (auto& result : completed)
    {
        _pendingNodes.erase(result.targetNode);

        if (result.type == GenerationType::Merge)
        {
            // Release children's patches before merging
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = result.targetNode->GetChild(i);
                if (child && child->HasPatch())
                    _patchPool.Release(child->ReleasePatch());
            }
            result.targetNode->Merge();
        }

        // Release existing patch (parent fallback for splits, or stale patch) to pool
        if (result.targetNode->HasPatch())
            _patchPool.Release(result.targetNode->ReleasePatch());

        result.targetNode->SetPatch(std::move(result.patch));
    }
}

void PlanetQuadTree::Update(const glm::vec3& cameraPos, const glm::mat4& viewProjection)
{
    _currentFrustum = Frustum::FromViewProjection(viewProjection);
    _activeLeafCount = 0;
    _totalNodeCount = 0;
    _lastCameraPos = cameraPos;

    for (auto& root : _roots)
    {
        TraverseAndUpdate(*root, cameraPos);
    }
}

void PlanetQuadTree::TraverseAndUpdate(QuadTreeNode& node, const glm::vec3& cameraPos)
{
    ++_totalNodeCount;

    glm::vec3 scaledCenter = node.GetCenter() * _config.planetRadius;
    float distance = glm::length(cameraPos - scaledCenter);
    float arcLength = node.GetArcLength();
    float splitDistance = _config.splitThreshold * arcLength * _config.planetRadius;

    if (node.IsLeaf())
    {
        bool shouldSplit = distance < splitDistance &&
                          node.CanSplit() &&
                          _activeLeafCount < _config.maxActivePatches;

        if (shouldSplit && !IsPending(&node))
        {
            // Split creates children but preserves parent patch as fallback
            node.Split();

            // Enqueue async generation for all children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                    EnqueueGeneration(*child, GenerationType::Split);
            }

            // Recurse into children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                    TraverseAndUpdate(*child, cameraPos);
            }
        }
        else
        {
            ++_activeLeafCount;
        }
    }
    else
    {
        // Interior node: check if all children are leaves and should merge
        bool allChildrenAreLeaves = true;
        for (int i = 0; i < 4; ++i)
        {
            QuadTreeNode* child = node.GetChild(i);
            if (child && !child->IsLeaf())
            {
                allChildrenAreLeaves = false;
                break;
            }
        }

        float mergeDistance = splitDistance * _config.hysteresis;
        bool shouldMerge = allChildrenAreLeaves && distance > mergeDistance;

        if (shouldMerge && !IsPending(&node))
        {
            // Enqueue parent patch generation; children stay alive until patch arrives
            EnqueueGeneration(node, GenerationType::Merge);

            // Count children as active while merge is pending
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                    ++_activeLeafCount;
            }
        }
        else
        {
            // Recurse into children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                    TraverseAndUpdate(*child, cameraPos);
            }
        }
    }
}

void PlanetQuadTree::EnqueueGeneration(QuadTreeNode& node, GenerationType type)
{
    if (IsPending(&node))
        return;

    auto patch = _patchPool.Acquire();
    patch->Initialize(
        node.GetVertex(0),
        node.GetVertex(1),
        node.GetVertex(2),
        _config.planetRadius,
        _config.meshResolution,
        _config.skirtFraction);

    float distance = glm::length(_lastCameraPos - node.GetCenter() * _config.planetRadius);

    _pendingNodes.insert(&node);

    GenerationRequest request;
    request.targetNode = &node;
    request.type = type;
    request.patch = std::move(patch);
    request.priority = distance;

    _scheduler->Enqueue(std::move(request));
}

bool PlanetQuadTree::IsPending(const QuadTreeNode* node) const
{
    return _pendingNodes.find(node) != _pendingNodes.end();
}

void PlanetQuadTree::Render(const Shader& shader) const
{
    std::vector<const QuadTreeNode*> renderableNodes;
    renderableNodes.reserve(_activeLeafCount);

    for (const auto& root : _roots)
    {
        CollectRenderableNodes(*root, renderableNodes);
    }

    int visibleCount = 0;
    for (const QuadTreeNode* node : renderableNodes)
    {
        const SpherePatch* patch = node->GetPatch();
        if (patch && patch->IsVisible(_currentFrustum))
        {
            patch->Render();
            ++visibleCount;
        }
    }

    _visiblePatchCount = visibleCount;
}

void PlanetQuadTree::CollectRenderableNodes(
    const QuadTreeNode& node,
    std::vector<const QuadTreeNode*>& nodes) const
{
    if (node.IsLeaf())
    {
        if (node.HasPatch())
            nodes.push_back(&node);
        return;
    }

    // If all leaves in subtree have patches, recurse for finer LOD
    if (IsSubtreeReady(node))
    {
        for (int i = 0; i < 4; ++i)
        {
            const QuadTreeNode* child = node.GetChild(i);
            if (child)
                CollectRenderableNodes(*child, nodes);
        }
    }
    else if (node.HasPatch())
    {
        // Fallback: render parent patch at coarser LOD while children generate
        nodes.push_back(&node);
    }
}

bool PlanetQuadTree::IsSubtreeReady(const QuadTreeNode& node) const
{
    if (node.IsLeaf())
        return node.HasPatch();

    for (int i = 0; i < 4; ++i)
    {
        const QuadTreeNode* child = node.GetChild(i);
        if (!child || !IsSubtreeReady(*child))
            return false;
    }
    return true;
}

} // namespace planets::render::lod
