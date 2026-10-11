#pragma once

namespace planets::render
{

class BodyRuntime;
struct SurfaceSettings;

// Planet window > Surface: materials, climate, surface rules and textures. Surface rules apply
// live; the climate model and shading noise feed the terrain compute, so they apply on release.
class SurfaceTab
{
public:
    // Returns true when the terrain must regenerate.
    bool Draw(BodyRuntime& body, SurfaceSettings& surface);
};

} // namespace planets::render
