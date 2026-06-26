#pragma once

#include <memory>

namespace planets::render
{

struct BiomeSettings;
struct EarthColors;
struct EarthShadingSettings;
class BodyRuntime;

namespace effects
{
struct OceanSettings;
}

class SurfacePanel
{
public:
    // Body-type-aware draw: shows Earth controls for "earth" typeName, palette controls otherwise
    bool Draw(BodyRuntime* activeBody,
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

    // Generic body sections (edits config shading block inline)
    bool DrawGenericSurfaceContent(BodyRuntime& body);

    // Shared sections
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);
};

} // namespace planets::render
