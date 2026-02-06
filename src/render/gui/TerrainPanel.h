#pragma once

#include <cstdint>

namespace planets::core {
class Planet;
}

namespace planets::render {

struct EarthTerrainSettings;
struct GenerationConfig;
struct LodConfig;
struct TerrainStats;

class TerrainPanel
{
public:
    // Returns true if planet needs regeneration
    bool Draw(GenerationConfig& config, EarthTerrainSettings& terrain,
              LodConfig& lod, const TerrainStats& stats,
              planets::core::Planet& planet, bool& visible);

private:
    void DrawGpuContent(GenerationConfig& config, const TerrainStats& stats);
    bool DrawEarthTerrainContent(EarthTerrainSettings& settings, uint32_t& seed, int& subdivisions);
    bool DrawPlanetContent(planets::core::Planet& planet);
    bool DrawLodContent(LodConfig& lod, const TerrainStats& stats);
};

} // namespace planets::render
