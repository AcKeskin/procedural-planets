#pragma once

#include "CinematicPanel.h"
#include "DebugPanel.h"
#include "LightTab.h"
#include "OceanTab.h"
#include "SkyTab.h"
#include "SurfaceTab.h"
#include "SystemTab.h"
#include "TerrainTab.h"
#include "../types/LookUnits.h"
#include <array>
#include <set>

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
    core::Camera* camera = nullptr; // the Camera tab edits FOV and auto-rotate
    float* moveSpeed = nullptr;
    bool autoOrbit = false;
    float* autoOrbitSpeed = nullptr;
    CinematicSettings* cinematic = nullptr;
    float planetRadius = 1.0f;
    float frameMs = 16.0f;

    const types::LookState* baseline = nullptr; // the rolled values edits are measured against
    uint32_t* lockMask = nullptr;               // one bit per LookUnit
    uint32_t varietyUnits = 0;                  // units the type can randomize
};

// What the user asked for this frame; the app carries it out.
struct PlanetActions
{
    bool regenerate = false;
    bool playCinematic = false;
    int randomizeUnit = -1; // a LookUnit, or -1
    int resetUnit = -1;
};

// The one docked window: tabs Terrain, Surface, Ocean, Sky, Light, Camera, System.
class PlanetWindow
{
public:
    static constexpr const char* WindowName = "Planet";

    PlanetActions Draw(PlanetUi& ui);

    // Edited values on all look tabs, as last drawn.
    size_t EditCount() const;
    // The values of these units were replaced; their edit records no longer hold.
    void ForgetEdits(uint32_t units);

private:
    // Lock, Randomize, Reset and the edited count at the top of a look tab.
    void Toolbar(types::LookUnit unit, const PlanetUi& ui, PlanetActions& actions);

    std::array<std::set<const void*>, static_cast<size_t>(types::LookUnit::Count)> _edits;

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
