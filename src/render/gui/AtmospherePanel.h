#pragma once

namespace planets::render::effects
{
struct AtmosphereSettings;
}

namespace planets::render
{

class AtmospherePanel
{
public:
    void DrawContent(effects::AtmosphereSettings& settings);
};

} // namespace planets::render
