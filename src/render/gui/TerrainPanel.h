#pragma once

#include <cstdint>

namespace planets::render
{

struct EarthTerrainSettings;
struct GenerationConfig;
struct LodConfig;
struct TerrainStats;

class TerrainPanel
{
public:
    // Returns true if planet needs regeneration
    bool Draw(GenerationConfig& config,
              EarthTerrainSettings& terrain,
              LodConfig& lod,
              const TerrainStats& stats,
              bool& visible,
              bool& randomizeRequested);

private:
    void DrawGpuContent(GenerationConfig& config, const TerrainStats& stats);
    bool DrawEarthTerrainContent(EarthTerrainSettings& settings,
                                 uint32_t& seed,
                                 int& subdivisions,
                                 bool& randomizeRequested);
    bool DrawLodContent(LodConfig& lod, const TerrainStats& stats);
};

} // namespace planets::render
