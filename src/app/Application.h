#pragma once

#include "Input.h"
#include "Window.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "TerrainGenerator.h"
#include "GenerationScheduler.h"
#include "ParameterRandomizer.h"
#include "Framebuffer.h"
#include "PostProcessor.h"
#include "lod/PlanetQuadTree.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
#include "gui/GuiManager.h"
#include "gui/ScenePanel.h"
#include "gui/TerrainPanel.h"
#include "gui/SurfacePanel.h"
#include "gui/AtmospherePanel.h"
#include "gui/DebugPanel.h"
#include "settings/SceneSettings.h"
#include "settings/TerrainSettings.h"
#include "settings/SurfaceSettings.h"
#include "settings/OceanSettings.h"
#include "settings/AtmosphereSettings.h"
#include "settings/CinematicSettings.h"
#include "BiomePalette.h"
#include "BodyRuntime.h"
#include "ContinentMaskRenderer.h"
#include <memory>
#include "cinematic/CinematicController.h"
#include "cinematic/CaptureManager.h"
#include "gui/CinematicPanel.h"
#include "math/Camera.h"
#include "CaptureRequest.h"
#include <model/PaletteRegistry.h>

namespace planets::app
{

namespace AppDefaults
{
inline constexpr float SeaLevel = 0.995f;
inline constexpr float MoveSpeed = 75.0f;
inline constexpr float InitialCameraDistanceMultiplier = 2.5f;
inline constexpr float MouseRotationSensitivity = 0.1f;
inline constexpr float SprintSpeedMultiplier = 5.0f;
inline constexpr float AutoOrbitSpeed = 5.0f;
inline constexpr float FresnelStrengthNear = 0.5f;
inline constexpr float FresnelStrengthFar = 2.0f;
inline constexpr float FresnelPower = 3.0f;
inline constexpr int SchedulerPatchesPerFrame = 4;
inline constexpr float MinFarPlane = 1000.0f;
inline constexpr float FarPlaneRadiusMultiplier = 20.0f;
inline constexpr int OceanSubdivisions = 5;
inline constexpr float HeightRangeMultiplier = 1.5f;
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
    void Run();
    void Shutdown();

    // Headless capture: boot with overrides, render warm-up frames, write one PNG, return.
    // Returns true on a successful capture.
    bool RunCapture(const CaptureRequest& request);

    // Apply CLI overrides before window/scene creation (size, vsync, seed, camera).
    void ApplyCaptureOverrides(const CaptureRequest& request);

private:
    void ProcessInput(float deltaTime);
    void Render();
    void RenderGui();

    void RegenerateLodSystem();
    void ShuffleTerrain();
    void StartCinematic();
    void StopCinematic();

    // Load a BodyConfig from data/bodies/<name>.json and switch to it
    void SwitchBody(planetgen::BodyConfig config);

    // Build QuadTreeConfig from current LodConfig
    render::lod::QuadTreeConfig BuildQuadTreeConfig() const;

    // Core systems
    render::Window _window;
    Input _input;
    render::Renderer _renderer;

    // Shaders
    render::Shader _planetShader;
    render::Shader _spaceShader;
    render::Shader _passthroughShader;

    // Rendering pipeline
    render::Framebuffer _sceneFbo;
    render::PostProcessor _postProcessor;
    render::TerrainGenerator _terrainGenerator;
    render::GenerationScheduler _generationScheduler;
    render::lod::PlanetQuadTree _quadTree;
    render::effects::OceanRenderer _oceanRenderer;
    render::effects::AtmosphereRenderer _atmosphereRenderer;

    // GUI
    render::GuiManager _guiManager;
    render::ScenePanel _scenePanel;
    render::TerrainPanel _terrainPanel;
    render::SurfacePanel _surfacePanel;
    render::AtmospherePanel _atmospherePanel;
    render::DebugPanel _debugPanel;

    // Settings (Application owns all)
    render::SceneSettings _sceneSettings;
    render::EarthTerrainSettings _terrainSettings;   // GUI terrain panel edits these for Earth
    render::EarthShadingSettings _shadingSettings;   // GUI surface panel edits these for Earth
    render::GenerationConfig _genConfig;
    render::LodConfig _lodConfig;
    render::TerrainStats _terrainStats;
    render::BiomeSettings _biomeSettings;            // GUI biome panel
    render::EarthColors _earthColors;                // GUI color panel
    render::effects::OceanSettings _oceanSettings;
    render::effects::AtmosphereSettings _atmosphereSettings;
    render::BiomePalette _biomePalette;
    render::GpuBuffer<render::BiomeEntry> _paletteBuffer;
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
    bool _cinematicPanelVisible = true;
    bool _screenshotRequested = false;

    // Active body — typed config + runtime shim
    planetgen::PaletteRegistry _paletteRegistry;
    std::unique_ptr<render::BodyRuntime> _activeBody;

    // Continental mask — generated once per regen when continent params change
    render::ContinentMaskRenderer _continentMaskRenderer;
    int   _lastContinentCount          = -1;
    float _lastContinentSizeVariance   = -1.0f;
    float _lastContinentClustering     = -1.0f;
    int   _lastContinentMaskResolution = -1;
    uint32_t _lastMaskSeed             = ~0u;
    int _selectedBodyType = 0; // GUI selector index

    // Frame state
    float _lastTime;
    float _deltaTime = 0.0f;
    int _previousWidth;
    int _previousHeight;

    // Headless capture overrides (applied in Initialize when .enabled). Default = interactive.
    CaptureRequest _capture;
    bool _headlessCaptureRequested = false;
};

} // namespace planets::app
