#pragma once

#include "SpherePatch.h"
#include <deque>
#include <memory>

namespace planets::render::lod
{

// Recycles SpherePatch GPU resources to avoid per-split allocation overhead
class PatchPool
{
public:
    explicit PatchPool(int maxCacheSize = 200);

    // Get a patch from pool (or create new if empty)
    std::unique_ptr<SpherePatch> Acquire();

    // Return a patch to the pool for later reuse
    void Release(std::unique_ptr<SpherePatch> patch);

    // Clear all cached patches
    void Clear();

    int GetCachedCount() const { return static_cast<int>(_cache.size()); }

private:
    std::deque<std::unique_ptr<SpherePatch>> _cache;
    int _maxCacheSize;
};

} // namespace planets::render::lod
