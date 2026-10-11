#pragma once

namespace planets::render
{

class BodyRuntime;
struct SurfaceSettings;

namespace effects
{
struct OceanSettings;
}

// Edits the active body's shading block (noise, climate, biomes, colours) directly
// on its BodyConfig — one generic path for every body type, no per-type branching.
class SurfacePanel
{
public:
    bool Draw(BodyRuntime* activeBody,
              SurfaceSettings& surface,
              effects::OceanSettings& ocean,
              float& seaLevel,
              bool& visible);

private:
    bool DrawShadingBlock(BodyRuntime& body);
    // Planet type traits: materials and climate (live), terrain styles (regenerate).
    bool DrawPlanetType(BodyRuntime& body, SurfaceSettings& surface);
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);
};

} // namespace planets::render
