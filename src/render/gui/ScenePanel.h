#pragma once

namespace planets::render
{

struct SceneSettings;

class ScenePanel
{
public:
    void Draw(SceneSettings& settings, bool& visible);

private:
    void DrawSpaceContent(SceneSettings& settings);
    void DrawLightingContent(struct LightingSettings& settings);
};

} // namespace planets::render
