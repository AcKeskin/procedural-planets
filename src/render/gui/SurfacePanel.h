#pragma once

#include <memory>

namespace planets::render
{

struct BiomeSettings;
struct EarthColors;
struct EarthShadingSettings;
class CelestialBody;
class GenericBody;
class Earth;

namespace effects
{
struct OceanSettings;
}

class SurfacePanel
{
public:
    // Body-type-aware draw — shows Earth controls for Earth, palette controls for generic bodies
    bool Draw(CelestialBody* activeBody,
              BiomeSettings& biomes,
              EarthColors& colors,
              EarthShadingSettings& shading,
              effects::OceanSettings& ocean,
              float& seaLevel,
              bool& visible);

private:
    // Earth-specific sections
    void DrawBiomeContent(BiomeSettings& settings);
    void DrawEarthColorsContent(EarthColors& colors);
    bool DrawShadingContent(EarthShadingSettings& settings);

    // Generic body sections
    bool DrawGenericSurfaceContent(GenericBody& body);

    // Shared sections
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);
};

} // namespace planets::render
