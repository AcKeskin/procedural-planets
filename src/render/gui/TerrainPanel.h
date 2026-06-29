#pragma once

#include <cstdint>

namespace planets::render
{

struct GenerationConfig;
struct LodConfig;
struct TerrainStats;
class BodyRuntime;

// Edits the active body's shape/tectonics/ocean-floor/height-detail/erosion blocks
// directly on its BodyConfig — one generic path for every body type.
class TerrainPanel
{
public:
    bool Draw(BodyRuntime* activeBody,
              GenerationConfig& config,
              LodConfig& lod,
              const TerrainStats& stats,
              bool& visible,
              bool& randomizeRequested);

private:
    void DrawGpuContent(const TerrainStats& stats);
    bool DrawTerrainBlocks(BodyRuntime& body,
                           uint32_t& seed,
                           int& subdivisions,
                           bool& randomizeRequested);
    bool DrawLodContent(LodConfig& lod, const TerrainStats& stats);
};

} // namespace planets::render
