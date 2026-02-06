#pragma once

namespace planets::render::effects {
struct AtmosphereSettings;
}

namespace planets::render {

class AtmospherePanel
{
public:
    void Draw(effects::AtmosphereSettings& settings, bool& visible);
};

} // namespace planets::render
