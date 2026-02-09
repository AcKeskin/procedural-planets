#pragma once

namespace planets::render
{

struct CinematicSettings;
struct TurntableSettings;

class CinematicPanel
{
public:
    void Draw(CinematicSettings& settings, float planetRadius, bool& playRequested, bool& visible);

private:
    void DrawTurntableContent(TurntableSettings& settings, float planetRadius);
};

} // namespace planets::render
