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
    TerrainGenerator& terrainGen,
    const EarthTerrainSettings& terrainSettings,
    const EarthShadingSettings& shadingSettings,
    uint32_t seed)
{
    _config = config;
    _terrainGen = &terrainGen;
    _terrainSettings = &terrainSettings;
    _shadingSettings = &shadingSettings;
    _seed = seed;

    // Clear pool to avoid stale patches from previous generation
    _patchPool.Clear();

    // Build root nodes from subdivided icosahedron
    BuildRootNodes(_config.baseSubdivisions);

    std::cout << "[PlanetQuadTree] Created " << _roots.size() << " root nodes" << std::endl;

    // Generate patches for all root nodes (they start as leaves)
    for (auto& root : _roots)
    {
        GeneratePatchForNode(*root);
    }

    std::cout << "[PlanetQuadTree] Initial patch generation complete" << std::endl;
}

void PlanetQuadTree::BuildRootNodes(int subdivisions)
{
    // Start with base icosahedron faces
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

    // Subdivide faces
    for (int s = 0; s < subdivisions; ++s)
    {
        std::vector<Face> newFaces;
        newFaces.reserve(faces.size() * 4);

        for (const auto& face : faces)
        {
            // Calculate midpoints on unit sphere
            glm::vec3 m01 = glm::normalize((face.v0 + face.v1) * 0.5f);
            glm::vec3 m12 = glm::normalize((face.v1 + face.v2) * 0.5f);
            glm::vec3 m20 = glm::normalize((face.v2 + face.v0) * 0.5f);

            // Create 4 new faces
            newFaces.push_back({ face.v0, m01, m20 });
            newFaces.push_back({ m01, face.v1, m12 });
            newFaces.push_back({ m20, m12, face.v2 });
            newFaces.push_back({ m01, m12, m20 });
        }

        faces = std::move(newFaces);
    }

    // Create root nodes from faces (depth 0, no parent)
    _roots.reserve(faces.size());
    for (const auto& face : faces)
    {
        _roots.push_back(std::make_unique<QuadTreeNode>(
            face.v0, face.v1, face.v2, 0, nullptr));
    }
}

void PlanetQuadTree::Update(const glm::vec3& cameraPos, const glm::mat4& viewProjection)
{
    _currentFrustum = Frustum::FromViewProjection(viewProjection);
    _activeLeafCount = 0;
    _totalNodeCount = 0;

    // Traverse each root tree
    for (auto& root : _roots)
    {
        TraverseAndUpdate(*root, cameraPos);
    }
}

void PlanetQuadTree::TraverseAndUpdate(QuadTreeNode& node, const glm::vec3& cameraPos)
{
    ++_totalNodeCount;

    // Scale center to planet surface
    glm::vec3 scaledCenter = node.GetCenter() * _config.planetRadius;
    float distance = glm::length(cameraPos - scaledCenter);
    float arcLength = node.GetArcLength();
    float splitDistance = _config.splitThreshold * arcLength * _config.planetRadius;

    if (node.IsLeaf())
    {
        // Check if leaf should split
        bool shouldSplit = distance < splitDistance &&
                          node.CanSplit() &&
                          _activeLeafCount < _config.maxActivePatches;

        if (shouldSplit)
        {
            // Split node into 4 children
            node.Split();

            // Generate patches for all children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                {
                    GeneratePatchForNode(*child);
                }
            }

            // Recurse into children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                {
                    TraverseAndUpdate(*child, cameraPos);
                }
            }
        }
        else
        {
            // Leaf stays as leaf
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

        if (shouldMerge)
        {
            // Return children's patches to pool before destroying them
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child && child->HasPatch())
                {
                    _patchPool.Release(child->ReleasePatch());
                }
            }

            // Merge children back into this node
            node.Merge();

            // Generate patch for this node (now a leaf)
            GeneratePatchForNode(node);

            ++_activeLeafCount;
        }
        else
        {
            // Recurse into children
            for (int i = 0; i < 4; ++i)
            {
                QuadTreeNode* child = node.GetChild(i);
                if (child)
                {
                    TraverseAndUpdate(*child, cameraPos);
                }
            }
        }
    }
}

void PlanetQuadTree::GeneratePatchForNode(QuadTreeNode& node)
{
    auto patch = _patchPool.Acquire();
    patch->Initialize(
        node.GetVertex(0),
        node.GetVertex(1),
        node.GetVertex(2),
        _config.planetRadius,
        _config.meshResolution,
        _config.skirtFraction);

    const auto& unitVertices = patch->GetUnitSphereVertices();
    auto heights = _terrainGen->GenerateHeights(unitVertices, _seed, *_terrainSettings);

    std::vector<glm::vec4> shadingData;
    if (_terrainGen->IsShadingReady())
    {
        shadingData = _terrainGen->GenerateShadingData(unitVertices, _seed, *_shadingSettings);
    }
    else
    {
        shadingData.resize(unitVertices.size(), glm::vec4(0.0f));
    }

    patch->GenerateMesh(heights, shadingData);
    node.SetPatch(std::move(patch));
}

void PlanetQuadTree::Render(const Shader& shader) const
{
    // Collect all leaf nodes
    std::vector<QuadTreeNode*> leaves;
    leaves.reserve(_activeLeafCount);

    for (const auto& root : _roots)
    {
        CollectLeaves(*root, leaves);
    }

    // Render visible patches
    int visibleCount = 0;
    for (QuadTreeNode* leaf : leaves)
    {
        const SpherePatch* patch = leaf->GetPatch();
        if (patch && patch->IsVisible(_currentFrustum))
        {
            patch->Render();
            ++visibleCount;
        }
    }

    _visiblePatchCount = visibleCount;
}

void PlanetQuadTree::CollectLeaves(QuadTreeNode& node, std::vector<QuadTreeNode*>& leaves) const
{
    if (node.IsLeaf())
    {
        leaves.push_back(&node);
    }
    else
    {
        for (int i = 0; i < 4; ++i)
        {
            QuadTreeNode* child = node.GetChild(i);
            if (child)
            {
                CollectLeaves(*child, leaves);
            }
        }
    }
}

} // namespace planets::render::lod
