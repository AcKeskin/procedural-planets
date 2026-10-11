#pragma once

namespace planets::render
{

struct CinematicSettings;
struct TurntableSettings;

class CinematicPanel
{
public:
    void DrawContent(CinematicSettings& settings, float planetRadius, bool& playRequested);

private:
    void DrawTurntableContent(TurntableSettings& settings, float planetRadius);
};

} // namespace planets::render
