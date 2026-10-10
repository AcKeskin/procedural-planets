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
    void Draw(effects::CloudSettings& settings, bool& visible);
};

} // namespace planets::render
