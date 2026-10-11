#pragma once

namespace planets::render
{

struct LodConfig;
struct TerrainStats;

// Planet window > System: frame time, GPU split, patch counts and the mesh detail (LOD) settings.
class SystemTab
{
public:
    // Returns true when the LOD system must be rebuilt.
    bool Draw(LodConfig& lod, const TerrainStats& stats, float frameMs);

private:
    static constexpr int HistorySize = 120;
    float _frameHistory[HistorySize] = {};
    int _historyHead = 0;
};

} // namespace planets::render
