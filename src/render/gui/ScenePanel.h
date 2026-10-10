#pragma once

namespace planets::render
{

struct SceneSettings;

class ScenePanel
{
public:
    // atmosphereOwnsHaze: the atmosphere pass is on, so the renderer zeroes the surface haze.
    void Draw(SceneSettings& settings, bool atmosphereOwnsHaze, bool& visible);

private:
    void DrawSpaceContent(SceneSettings& settings);
    void DrawLightingContent(struct LightingSettings& settings, bool atmosphereOwnsHaze);
};

} // namespace planets::render
