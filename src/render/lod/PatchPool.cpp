#include "PatchPool.h"

namespace planets::render::lod
{

PatchPool::PatchPool(int maxCacheSize)
    : _maxCacheSize(maxCacheSize)
{
}

std::unique_ptr<SpherePatch> PatchPool::Acquire()
{
    if (_cache.empty())
        return std::make_unique<SpherePatch>();

    auto patch = std::move(_cache.front());
    _cache.pop_front();
    return patch;
}

void PatchPool::Release(std::unique_ptr<SpherePatch> patch)
{
    if (!patch)
        return;

    // Evict oldest if at capacity
    if (static_cast<int>(_cache.size()) >= _maxCacheSize)
        _cache.pop_front();

    _cache.push_back(std::move(patch));
}

void PatchPool::Clear()
{
    _cache.clear();
}

} // namespace planets::render::lod
