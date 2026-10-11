#pragma once

#include "CinematicPanel.h"
#include "DebugPanel.h"
#include "LightTab.h"
#include "OceanTab.h"
#include "SkyTab.h"
#include "SurfaceTab.h"
#include "SystemTab.h"
#include "TerrainTab.h"

namespace planets::core
{
class Camera;
}

namespace planets::render
{

class BodyRuntime;
struct SceneSettings;
struct SurfaceSettings;
struct GenerationConfig;
struct LodConfig;
struct TerrainStats;
struct CinematicSettings;
namespace effects
{
struct AtmosphereSettings;
struct CloudSettings;
struct OceanSettings;
} // namespace effects

// Everything the Planet window edits, owned by the app. Pointers are never null while the
// window draws.
struct PlanetUi
{
    BodyRuntime* body = nullptr;
    GenerationConfig* gen = nullptr;
    LodConfig* lod = nullptr;
    const TerrainStats* stats = nullptr;
    SurfaceSettings* surface = nullptr;
    effects::OceanSettings* ocean = nullptr;
    float* seaLevel = nullptr;
    effects::AtmosphereSettings* atmosphere = nullptr;
    effects::CloudSettings* clouds = nullptr;
    SceneSettings* scene = nullptr;
    const core::Camera* camera = nullptr;
    float* moveSpeed = nullptr;
    bool autoOrbit = false;
    float* autoOrbitSpeed = nullptr;
    CinematicSettings* cinematic = nullptr;
    float planetRadius = 1.0f;
    float frameMs = 16.0f;
};

// What the user asked for this frame; the app carries it out.
struct PlanetActions
{
    bool regenerate = false;
    bool randomize = false;
    bool playCinematic = false;
};

// The one docked window: tabs Terrain, Surface, Ocean, Sky, Light, Camera, System.
class PlanetWindow
{
public:
    static constexpr const char* WindowName = "Planet";

    PlanetActions Draw(PlanetUi& ui);

private:
    TerrainTab _terrain;
    SystemTab _system;
    SurfaceTab _surfaceTab;
    OceanTab _ocean;
    SkyTab _sky;
    LightTab _light;
    DebugPanel _debug;
    CinematicPanel _cinematic;
};

} // namespace planets::render
