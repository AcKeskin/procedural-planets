#include "QuadTreeNode.h"
#include "SpherePatch.h"
#include <glm/geometric.hpp>
#include <glm/common.hpp>
#include <algorithm>
#include <cmath>

namespace planets::render::lod
{

QuadTreeNode::QuadTreeNode(
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, int depth, QuadTreeNode* parent)
    : _v0(glm::normalize(v0))
    , _v1(glm::normalize(v1))
    , _v2(glm::normalize(v2))
    , _center(glm::normalize((_v0 + _v1 + _v2) / 3.0f))
    , _arcLength(0.0f)
    , _depth(depth)
    , _parent(parent)
    , _children{}
    , _patch(nullptr)
{
    // Compute maximum angular distance from center to any corner
    float maxAngle = 0.0f;
    for (const glm::vec3& vertex : {_v0, _v1, _v2})
    {
        float cosAngle = std::clamp(glm::dot(_center, vertex), -1.0f, 1.0f);
        float angle = std::acos(cosAngle);
        maxAngle = std::max(maxAngle, angle);
    }
    _arcLength = maxAngle;
}

bool QuadTreeNode::ShouldSplit(const glm::vec3& cameraPos, float splitThreshold) const
{
    if (!IsLeaf())
        return false;

    float distance = glm::length(cameraPos - _center);
    return distance < splitThreshold * _arcLength;
}

bool QuadTreeNode::ShouldMerge(const glm::vec3& cameraPos, float splitThreshold, float hysteresis) const
{
    if (IsLeaf())
        return false;

    // All children must be leaves
    for (const auto& child : _children)
    {
        if (child && !child->IsLeaf())
            return false;
    }

    float distance = glm::length(cameraPos - _center);
    return distance > splitThreshold * _arcLength * hysteresis;
}

void QuadTreeNode::Split()
{
    if (!CanSplit())
        return;

    // Compute edge midpoints and normalize to unit sphere
    glm::vec3 m01 = glm::normalize((_v0 + _v1) * 0.5f);
    glm::vec3 m12 = glm::normalize((_v1 + _v2) * 0.5f);
    glm::vec3 m20 = glm::normalize((_v2 + _v0) * 0.5f);

    // Create four child triangles via edge bisection
    _children[0] = std::make_unique<QuadTreeNode>(_v0, m01, m20, _depth + 1, this);
    _children[1] = std::make_unique<QuadTreeNode>(m01, _v1, m12, _depth + 1, this);
    _children[2] = std::make_unique<QuadTreeNode>(m20, m12, _v2, _depth + 1, this);
    _children[3] = std::make_unique<QuadTreeNode>(m01, m12, m20, _depth + 1, this);

    // Parent patch is preserved as fallback while children generate asynchronously
}

void QuadTreeNode::Merge()
{
    if (IsLeaf())
        return;

    // Destroy all children via reset
    for (auto& child : _children)
    {
        child.reset();
    }
}

void QuadTreeNode::SetPatch(std::unique_ptr<SpherePatch> patch)
{
    _patch = std::move(patch);
}

std::unique_ptr<SpherePatch> QuadTreeNode::ReleasePatch()
{
    return std::move(_patch);
}

const glm::vec3& QuadTreeNode::GetVertex(int index) const
{
    switch (index)
    {
    case 0:
        return _v0;
    case 1:
        return _v1;
    case 2:
        return _v2;
    default:
        return _v0; // Fallback to v0 for out-of-range indices
    }
}

} // namespace planets::render::lod
