#pragma once

namespace planets::render
{

class BodyRuntime;
struct SurfaceSettings;
namespace effects
{
struct OceanSettings;
}

// Planet window > Ocean: sea level, water colour, waves, fluid (lava, ice, acid) and seabed colour.
class OceanTab
{
public:
    // Returns true when the terrain must regenerate (sea level changed).
    bool Draw(BodyRuntime& body, effects::OceanSettings& ocean, float& seaLevel, SurfaceSettings& surface);
};

} // namespace planets::render
