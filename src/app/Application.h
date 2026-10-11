#pragma once

#include "Input.h"
#include "Bookmarks.h"
#include "HelpOverlay.h"
#include "Toast.h"
#include "Window.h"
#include "Renderer.h"
#include "lod/PlanetQuadTree.h"
#include "gui/GuiManager.h"
#include "gui/PlanetWindow.h"
#include "types/BodyRegistry.h"
#include "types/BodyLook.h"
#include "types/LookUnits.h"
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
#include "math/Camera.h"
#include "math/CameraController.h"
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
inline constexpr float InitialCameraDistanceMultiplier = 2.0f;
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

    // Short message in the bottom-right corner (about 2.5 s).
    void ShowToast(std::string text) { _toasts.Show(std::move(text)); }

    // Pin capture-mode lighting exposure (re-applied after a showcase regenerate).
    void ApplyCaptureExposure();

private:
    void ProcessInput(float deltaTime);
    // Camera input for this frame (keys, drags, wheel), then the controller update.
    void UpdateCamera(float deltaTime, bool keysBlocked);
    // Double-click: fly to the sea-level point under the cursor, if the ray hits the planet.
    void FlyToCursor();
    void SaveBookmark(int slot);
    void RecallBookmark(int slot);
    core::CameraBody CameraBodyNow() const;

    // Esc: close help, else stop the turntable; held for a second it quits.
    void HandleEscape(float deltaTime, bool keysBlocked);
    void Render();
    void RenderGui();
    // Main menu bar: View menu, type, seed, reroll, frame time. Visible even when Tab hides the
    // Planet window.
    void DrawBar();

    void RegenerateLodSystem();
    // New world: a new seed; the unlocked tabs take the type's values at it.
    void ShuffleTerrain();

    // Resize the renderer's framebuffer to the current window size if they differ (cheap
    // no-op when unchanged). Must run before any render that may be captured in live mode.
    void SyncFramebufferToWindow();

    // Aim the world light from over the camera's shoulder so the lit hemisphere faces the
    // viewer (used per-frame in the cinematic when lightFollow is on).
    void UpdateFollowLight();

    // The values the look tabs edit, as they are now.
    render::types::LookState CaptureLook() const;
    // Makes `s` live. rebuildBody: new runtime + library body + LOD (terrain, surface or sea level changed).
    void ApplyLook(const render::types::LookState& s, bool rebuildBody, bool resetCamera);
    // Rolls the active type at `seed`; only `units` take the roll, the other tabs keep their values.
    bool RollWorld(uint32_t seed, uint32_t units, bool resetCamera);
    // Switches to the body JSON at path, its variety rolled with the current seed.
    bool SwitchBodyByPath(const std::string& path, bool resetCamera = true);
    // After a seed change: rolls the active type's variety again. False when it has none.
    bool RerollActiveBody();
    // After a seed change: the unlocked tabs take the type's values at the new seed.
    bool RerollWorld();
    // One tab from a fresh roll (the seed stays) / back to the type's values at the current seed.
    void RandomizeUnit(render::types::LookUnit unit);
    void ResetUnit(render::types::LookUnit unit);

    // Build QuadTreeConfig from current LodConfig
    render::lod::QuadTreeConfig BuildQuadTreeConfig() const;
    float PlanetRadius() const; // the active body's (its config owns it)

    // Core systems
    render::Window _window;
    Input _input;
    render::Renderer _renderer; // owns the draw + LOD pipeline

    // GUI
    render::GuiManager _guiManager;
    render::PlanetWindow _planetWindow;
    float _frameMs = 16.0f; // smoothed frame time for the bar

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

    // Camera. Auto-rotate lives in _camera.Hud().autoRotate (the O key and the Camera tab share it).
    core::Camera _camera;
    core::CameraController _cameraCtl;
    Bookmarks _bookmarks;
    bool _orbitDragging = false; // LMB drag that started on the scene, not on a panel
    bool _lookDragging = false;  // same for RMB
    bool _leftWasDown = false;
    bool _rightWasDown = false;

    // Cinematic
    render::CinematicController _cinematicController;
    render::CinematicSettings _cinematicSettings;
    render::CaptureManager _captureManager;
    bool _guiVisible = true;
    bool _screenshotRequested = false;

    HelpOverlay _help;
    Toasts _toasts;
    float _escHeld = 0.0f;       // seconds Esc has been held toward quit
    bool _escConsumed = false;   // this press already closed help / stopped the turntable

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
    uint32_t _varietyUnits = 0;                    // look tabs its type rolls from the seed
    std::vector<glm::vec3> _activePalette;         // its type's palette (empty: the registry's)
    render::types::LookState _baseline;            // the rolled values; edits are measured against it
    uint32_t _lockMask = 0;                        // look tabs a reroll leaves alone
    uint32_t _offSeedMask = 0;                     // look tabs randomized away from the seed
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
