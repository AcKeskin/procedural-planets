#pragma once

namespace planets::render
{

struct SceneSettings;
struct SurfaceSettings;

// Planet window > Light: sun, ambient and shadow fill, twilight and night lights, stars, exposure.
class LightTab
{
public:
    void Draw(SceneSettings& scene, SurfaceSettings& surface);
};

} // namespace planets::render
