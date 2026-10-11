#pragma once

#include "Input.h"
#include "Window.h"
#include "Renderer.h"
#include "ParameterRandomizer.h"
#include "lod/PlanetQuadTree.h"
#include "gui/GuiManager.h"
#include "gui/ScenePanel.h"
#include "gui/TerrainPanel.h"
#include "gui/SurfacePanel.h"
#include "gui/AtmospherePanel.h"
#include "gui/DebugPanel.h"
#include "gui/CloudsPanel.h"
#include "types/BodyRegistry.h"
#include "types/BodyLook.h"
#include "settings/SceneSettings.h"
#include "settings/TerrainSettings.h"
#include "settings/OceanSettings.h"
#include "settings/AtmosphereSettings.h"
#include "settings/CloudSettings.h"
#include "settings/CinematicSettings.h"
#include "BiomePalette.h"
#include "BodyRuntime.h"
#include <planetgen/planetgen.hpp>
#include <memory>
#include "cinematic/CinematicController.h"
#include "cinematic/CaptureManager.h"
#include "gui/CinematicPanel.h"
#include "math/Camera.h"
#include "CaptureRequest.h"
#include "planetgen/model/PaletteRegistry.h"

namespace planets::app
{

namespace instrument
{
class ScriptRunner;
}

namespace AppDefaults
{
inline constexpr float SeaLevel = 0.995f;
inline constexpr float MoveSpeed = 75.0f;
inline constexpr float InitialCameraDistanceMultiplier = 2.0f;
inline constexpr float MouseRotationSensitivity = 0.1f;
inline constexpr float SprintSpeedMultiplier = 5.0f;
inline constexpr float AutoOrbitSpeed = 5.0f;
inline constexpr float FresnelStrengthNear = 0.5f;
inline constexpr float FresnelStrengthFar = 2.0f;
inline constexpr float FresnelPower = 3.0f;
inline constexpr int SchedulerPatchesPerFrame = 4;
inline constexpr float MinFarPlane = 1000.0f;
inline constexpr float FarPlaneRadiusMultiplier = 20.0f;
inline constexpr float HeightRangeMultiplier = 1.5f;
inline constexpr int CaptureDrainMaxFrames = 240;  // safety cap so a stuck queue can't hang capture
inline constexpr float CaptureSunIntensity = 1.6f; // brighter hero exposure for captures only
inline constexpr float CaptureAmbientLight = 0.40f;
} // namespace AppDefaults

// Owns all state, systems, and the frame loop
class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize();
    void Run(); // the interactive main loop, until the window closes
    void Shutdown();

    // Frame and scene controls the run modes (src/app/modes) drive the app with.

    // Initialize with CLI overrides (window size, seed, body, light, camera framing).
    bool Boot(const CaptureRequest& request);
    bool ShouldClose() const;
    // Poll events and input, move the camera, keep the framebuffer at the window size.
    void BeginInteractiveFrame();
    void PresentFrame(); // render + swap
    // Render one frame and write it as a PNG (to path, or the request's output path).
    // True when the capture ran.
    bool CaptureFrame(const std::string& path = {});

    // Render frames until the LOD scheduler has no pending/in-flight work, so a capture
    // never shoots a half-generated patch. minFrames forces a settle floor; maxFrames caps
    // the wait so a stuck queue can't hang. Returns true if it drained before the cap.
    bool DrainGeneration(int minFrames, int maxFrames);

    render::TurntableSettings& Turntable() { return _cinematicSettings.turntable; }
    void StartCinematic();
    void StopCinematic();
    // Advance the turntable by a fixed dt (deterministic captures), with the follow light.
    void StepCinematic(float dt);

    const std::vector<render::types::BodyEntry>& Bodies() const { return _bodies; }
    const std::string& ActiveBodyId() const { return _activeBodyId; }
    // Switches to a planet type from the registry; false (and a log line) when the id is unknown.
    bool SwitchBodyById(const std::string& id);
    void SetSeed(uint32_t seed) { _genConfig.seed = seed; }

    // Pin capture-mode lighting exposure (re-applied after a showcase regenerate).
    void ApplyCaptureExposure();

private:
    void ProcessInput(float deltaTime);
    void Render();
    void RenderGui();

    void RegenerateLodSystem();
    // New world: a planet type with a variety block rolls a new seed; others (Earth) shuffle
    // their generation parameters.
    void ShuffleTerrain();

    // Resize the renderer's framebuffer to the current window size if they differ (cheap
    // no-op when unchanged). Must run before any render that may be captured in live mode.
    void SyncFramebufferToWindow();

    // Aim the world light from over the camera's shoulder so the lit hemisphere faces the
    // viewer (used per-frame in the cinematic when lightFollow is on).
    void UpdateFollowLight();

    // Makes this body active and applies its look (sky, clouds, ocean, star).
    void SwitchBody(planetgen::BodyConfig config, const render::types::BodyLook& look = {}, bool resetCamera = true);
    // Switches to the body JSON at path, its variety rolled with the current seed.
    bool SwitchBodyByPath(const std::string& path, bool resetCamera = true);
    // After a seed change: rolls the active type's variety again. False when it has none.
    bool RerollActiveBody();

    // Build QuadTreeConfig from current LodConfig
    render::lod::QuadTreeConfig BuildQuadTreeConfig() const;
    float PlanetRadius() const; // the active body's (its config owns it)

    // Core systems
    render::Window _window;
    Input _input;
    render::Renderer _renderer; // owns the draw + LOD pipeline

    // GUI
    render::GuiManager _guiManager;
    render::ScenePanel _scenePanel;
    render::TerrainPanel _terrainPanel;
    render::SurfacePanel _surfacePanel;
    render::AtmospherePanel _atmospherePanel;
    render::DebugPanel _debugPanel;
    render::CloudsPanel _cloudsPanel;

    // App-visual settings (generation params live in the active body's BodyConfig).
    render::SceneSettings _sceneSettings;
    render::GenerationConfig _genConfig;
    render::LodConfig _lodConfig;
    render::TerrainStats _terrainStats;
    render::effects::OceanSettings _oceanSettings;
    render::effects::AtmosphereSettings _atmosphereSettings;
    render::effects::CloudSettings _cloudSettings;
    render::SurfaceSettings _surfaceSettings;
    float _seaLevel;

    // Camera
    core::Camera _camera;
    float _moveSpeed;
    bool _autoOrbit = false;
    float _autoOrbitSpeed = AppDefaults::AutoOrbitSpeed;

    // Cinematic
    render::CinematicController _cinematicController;
    render::CinematicPanel _cinematicPanel;
    render::CinematicSettings _cinematicSettings;
    render::CaptureManager _captureManager;
    bool _guiVisible = true;
    bool _screenshotRequested = false;

    // Active body — typed config + runtime shim
    planetgen::PaletteRegistry _paletteRegistry;
    std::unique_ptr<render::BodyRuntime> _activeBody;

    // libplanetgen context (owns GPU compute; the app generates terrain ONLY through
    // this) + the active body handle, recreated on SwitchBody. Context uses the app's
    // existing GL context so per-patch dispatch hits the app's LOD buffers.
    std::unique_ptr<pg::Context> _libContext;
    pg::Body _libBody;

    std::vector<render::types::BodyEntry> _bodies; // planet types found in data/bodies
    int _selectedBodyType = 0;                     // index into _bodies
    std::string _activeBodyId;                     // registry id of the active body
    std::string _activeBodyPath;                   // JSON the active body came from
    bool _activeHasVariety = false;                // its type rolls traits from the seed
    uint32_t _rolledSeed = 0;                      // seed the active world's traits were rolled with

    // Frame state
    float _lastTime;
    int _previousWidth;
    int _previousHeight;

    // Headless capture overrides (applied in Initialize when .enabled). Default = interactive.
    CaptureRequest _capture;
    bool _headlessCaptureRequested = false;

    // The instrument runner is an extension of the run modes; it mutates the private
    // settings members + drives Render/Drain/Regenerate directly rather than widening the
    // public surface for every tunable.
    friend class instrument::ScriptRunner;
};

} // namespace planets::app
