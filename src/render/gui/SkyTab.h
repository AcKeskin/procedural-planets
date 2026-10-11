#pragma once

namespace planets::render
{

struct LightingSettings;
namespace effects
{
struct AtmosphereSettings;
struct CloudSettings;
} // namespace effects

// Planet window > Sky: atmosphere, one haze group and clouds. Disabled on airless bodies.
class SkyTab
{
public:
    void Draw(effects::AtmosphereSettings& atmosphere,
              effects::CloudSettings& clouds,
              LightingSettings& lighting,
              bool bodyHasAir);
};

} // namespace planets::render
