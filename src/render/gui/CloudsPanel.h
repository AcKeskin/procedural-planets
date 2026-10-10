#pragma once

namespace planets::render
{

namespace effects
{
struct CloudSettings;
}

class CloudsPanel
{
public:
    // atmosphereOn false greys the panel out: clouds are part of the atmosphere and hide with it.
    void Draw(effects::CloudSettings& settings, bool atmosphereOn, bool& visible);
};

} // namespace planets::render
