#pragma once

#include <glm/glm.hpp>
#include <array>
#include <memory>

namespace planets::render::lod
{

// Forward declaration to avoid circular dependency
class SpherePatch;

// Node in the adaptive quadtree LOD hierarchy
// Represents a spherical triangle that can recursively subdivide
class QuadTreeNode
{
public:
    static constexpr int MaxDepth = 8;

    // Construct node from three triangle vertices on unit sphere
    QuadTreeNode(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, int depth, QuadTreeNode* parent);

    ~QuadTreeNode() = default;

    QuadTreeNode(const QuadTreeNode&) = delete;
    QuadTreeNode& operator=(const QuadTreeNode&) = delete;

    QuadTreeNode(QuadTreeNode&&) noexcept = default;
    QuadTreeNode& operator=(QuadTreeNode&&) noexcept = default;

    // State queries
    bool IsLeaf() const { return _children[0] == nullptr; }
    bool CanSplit() const { return IsLeaf() && _depth < MaxDepth; }

    // Decide whether to split based on camera distance
    bool ShouldSplit(const glm::vec3& cameraPos, float splitThreshold) const;

    // Decide whether to merge based on camera distance and hysteresis
    bool ShouldMerge(const glm::vec3& cameraPos, float splitThreshold, float hysteresis) const;

    // Tree mutation
    void Split();
    void Merge();

    // Patch ownership (leaf nodes only)
    void SetPatch(std::unique_ptr<SpherePatch> patch);
    SpherePatch* GetPatch() { return _patch.get(); }
    const SpherePatch* GetPatch() const { return _patch.get(); }
    std::unique_ptr<SpherePatch> ReleasePatch();
    bool HasPatch() const { return _patch != nullptr; }

    // Geomorph "birth" ramp: 1 right after a split-child gets its patch, decayed toward 0 each
    // frame. A freshly generated fine patch starts collapsed onto the parent shape and eases to
    // full detail, so a fast approach can't snap it in at full displacement.
    void BeginMorphIn() { _morphIn = 1.0f; }
    void DecayMorphIn(float amount) { _morphIn = (_morphIn > amount) ? _morphIn - amount : 0.0f; }
    float GetMorphIn() const { return _morphIn; }

    // Accessors
    int GetDepth() const { return _depth; }
    const glm::vec3& GetCenter() const { return _center; }
    float GetArcLength() const { return _arcLength; }
    QuadTreeNode* GetParent() { return _parent; }
    const QuadTreeNode* GetParent() const { return _parent; }
    QuadTreeNode* GetChild(int index) { return _children[index].get(); }
    const QuadTreeNode* GetChild(int index) const { return _children[index].get(); }
    const glm::vec3& GetVertex(int index) const;

private:
    // Triangle corners on unit sphere
    glm::vec3 _v0, _v1, _v2;

    // Normalized centroid of triangle
    glm::vec3 _center;

    // Maximum angular extent from center to any corner
    float _arcLength;

    // Depth in tree (0 = root icosahedron face)
    int _depth;

    // Non-owning pointer to parent node
    QuadTreeNode* _parent;

    // Four child nodes (empty if leaf)
    std::array<std::unique_ptr<QuadTreeNode>, 4> _children;

    // Renderable patch (only valid for leaf nodes)
    std::unique_ptr<SpherePatch> _patch;

    // Birth ramp for geomorph-in (see BeginMorphIn)
    float _morphIn = 0.0f;
};

} // namespace planets::render::lod
