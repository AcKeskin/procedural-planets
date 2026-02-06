#pragma once

namespace planets::render {

struct BiomeSettings;
struct EarthColors;
struct EarthShadingSettings;

namespace effects {
struct OceanSettings;
}

class SurfacePanel
{
public:
    void Draw(BiomeSettings& biomes, EarthColors& colors,
              EarthShadingSettings& shading,
              effects::OceanSettings& ocean, float& seaLevel, bool& visible);

private:
    void DrawBiomeContent(BiomeSettings& settings);
    void DrawEarthColorsContent(EarthColors& colors);
    void DrawShadingContent(EarthShadingSettings& settings);
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);
};

} // namespace planets::render
