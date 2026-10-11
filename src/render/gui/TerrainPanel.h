#pragma once

#include <cstdint>

namespace planets::render
{

struct GenerationConfig;
struct LodConfig;
struct TerrainStats;
class BodyRuntime;

// Edits the active body's shape/tectonics/ocean-floor/height-detail/macro-world blocks
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
    bool DrawTerrainBlocks(BodyRuntime& body, uint32_t& seed, bool& randomizeRequested);
    bool DrawLodContent(BodyRuntime* body, LodConfig& lod, const TerrainStats& stats);
};

} // namespace planets::render
