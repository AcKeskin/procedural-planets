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
    bool DrawContent(BodyRuntime* activeBody,
              SurfaceSettings& surface,
              effects::OceanSettings& ocean,
              float& seaLevel);

    // Water: its own tab in the Planet window.
    void DrawOceanContent(effects::OceanSettings& settings, float& seaLevel);

private:
    bool DrawShadingBlock(BodyRuntime& body);
    // Planet type traits: materials and climate (live), terrain styles (regenerate).
    bool DrawPlanetType(BodyRuntime& body, SurfaceSettings& surface);
    // Every rule of the surface shader, grouped by material (live, no regenerate).
    void DrawSurfaceRules(SurfaceSettings& surface);
};

} // namespace planets::render
