#include "Application.h"
#include "GpuConstants.h"
#include <model/BodyConfigJson.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <iostream>
#include <chrono>
#include <random>

namespace planets::app
{

namespace
{

// Load a BodyConfig from a JSON file; crash-safe fallback returns default
planetgen::BodyConfig LoadBodyConfig(const std::string& path)
{
    planetgen::BodyConfig cfg;
    std::string err = planetgen::BodyConfigLoadFile(path, cfg);
    if (!err.empty())
        std::cerr << "[Application] Failed to load " << path << ": " << err << std::endl;
    return cfg;
}

// Sync application-owned Earth GUI settings into the body's BodyConfig.
// Earth is identified by typeName == "earth".
void SyncEarthToConfig(planetgen::BodyConfig& cfg,
                       const render::EarthTerrainSettings& t,
                       const render::EarthShadingSettings& s,
                       const render::BiomeSettings& b,
                       const render::EarthColors& c)
{
    auto& sh = cfg.shape;
    sh.continentNoise  = { t.continentScale, t.continentOctaves, t.continentPersistence,
                            t.continentLacunarity, t.continentStrength };
    sh.mountainNoise   = { t.mountainScale, t.mountainOctaves, t.mountainPersistence,
                            t.mountainLacunarity, t.mountainStrength };
    sh.maskNoise       = { t.maskScale, t.maskOctaves, t.maskPersistence,
                            t.maskLacunarity, 1.0f };
    sh.heightScale             = t.heightScale;
    sh.continentCount          = t.continentCount;
    sh.continentSizeVariance   = t.continentSizeVariance;
    sh.continentClustering     = t.continentClustering;
    sh.continentMaskResolution = t.continentMaskResolution;
    sh.oceanDepthMultiplier = t.oceanDepthMultiplier;
    sh.oceanFloorDepth      = t.oceanFloorDepth;
    sh.oceanFloorSmoothing  = t.oceanFloorSmoothing;
    sh.mountainBlend        = t.mountainBlend;
    sh.continentBaseLevel   = t.continentBaseLevel;
    sh.globalFrequency      = t.globalFrequency;
    sh.normalEpsilon        = t.normalEpsilon;
    sh.mountainPower        = t.mountainPower;
    sh.mountainGain         = t.mountainGain;
    sh.mountainSmoothing    = t.mountainSmoothing;

    auto& tec = cfg.tectonics;
    tec.enabled               = t.useTectonics;
    tec.numPlates             = t.numPlates;
    tec.continentalFraction   = t.continentalFraction;
    tec.boundaryWidth         = t.boundaryWidth;
    tec.convergentMountainScale = t.convergentMountainScale;
    tec.divergentRiftDepth    = t.divergentRiftDepth;
    tec.coastlineNoise        = t.coastlineNoise;
    tec.plateElevationNoise   = t.plateElevationNoise;

    auto& ofl = cfg.oceanFloor;
    ofl.enabled           = t.useOceanFloor;
    ofl.shelfWidth        = t.shelfWidth;
    ofl.oceanRidgeOctaves = t.oceanRidgeOctaves;
    ofl.oceanRidgeScale   = t.oceanRidgeScale;
    ofl.oceanRidgeStrength = t.oceanRidgeStrength;
    ofl.oceanRidgePower   = t.oceanRidgePower;
    ofl.oceanRidgeGain    = t.oceanRidgeGain;
    ofl.trenchOctaves     = t.trenchOctaves;
    ofl.trenchScale       = t.trenchScale;
    ofl.trenchDepth       = t.trenchDepth;
    ofl.abyssalOctaves    = t.abyssalOctaves;
    ofl.abyssalScale      = t.abyssalScale;
    ofl.abyssalStrength   = t.abyssalStrength;

    auto& hd = cfg.heightDetail;
    hd.detailLowThreshold  = t.detailLowThreshold;
    hd.detailHighThreshold = t.detailHighThreshold;
    hd.perturbStrengthLow  = t.perturbStrengthLow;
    hd.perturbStrengthHigh = t.perturbStrengthHigh;
    hd.detailOctavesLow    = t.detailOctavesLow;
    hd.detailOctavesHigh   = t.detailOctavesHigh;
    hd.detailPersistence   = t.detailPersistence;
    hd.detailLacunarity    = t.detailLacunarity;
    hd.perturbScale        = t.perturbScale;

    auto& er = cfg.erosion;
    er.enabled         = t.enableErosion;
    er.iterations      = t.erosionIterations;
    er.thermalRate     = t.thermalErosionRate;
    er.thermalThreshold = t.thermalThreshold;
    er.hydraulicRate   = t.hydraulicErosionRate;
    er.depositionRate  = t.depositionRate;
    er.evaporationRate = t.evaporationRate;

    // Shading/biome/colour fields are edited directly on the config by SurfacePanel —
    // no loose-struct mirror to sync here. (Terrain fields above remain until
    // TerrainPanel is converted the same way.)
}

// Populate app-owned GUI settings from a BodyConfig (on body switch for earth)
void SyncConfigToEarth(const planetgen::BodyConfig& cfg,
                       render::EarthTerrainSettings& t,
                       render::EarthShadingSettings& s,
                       render::BiomeSettings& b,
                       render::EarthColors& c)
{
    const auto& sh = cfg.shape;
    t.continentScale       = sh.continentNoise.scale;
    t.continentOctaves     = sh.continentNoise.octaves;
    t.continentPersistence = sh.continentNoise.persistence;
    t.continentLacunarity  = sh.continentNoise.lacunarity;
    t.continentStrength    = sh.continentNoise.strength;
    t.mountainScale        = sh.mountainNoise.scale;
    t.mountainOctaves      = sh.mountainNoise.octaves;
    t.mountainPersistence  = sh.mountainNoise.persistence;
    t.mountainLacunarity   = sh.mountainNoise.lacunarity;
    t.mountainStrength     = sh.mountainNoise.strength;
    t.maskScale            = sh.maskNoise.scale;
    t.maskOctaves          = sh.maskNoise.octaves;
    t.maskPersistence      = sh.maskNoise.persistence;
    t.maskLacunarity       = sh.maskNoise.lacunarity;
    t.heightScale             = sh.heightScale;
    t.continentCount          = sh.continentCount;
    t.continentSizeVariance   = sh.continentSizeVariance;
    t.continentClustering     = sh.continentClustering;
    t.continentMaskResolution = sh.continentMaskResolution;
    t.oceanDepthMultiplier = sh.oceanDepthMultiplier;
    t.oceanFloorDepth      = sh.oceanFloorDepth;
    t.oceanFloorSmoothing  = sh.oceanFloorSmoothing;
    t.mountainBlend        = sh.mountainBlend;
    t.continentBaseLevel   = sh.continentBaseLevel;
    t.globalFrequency      = sh.globalFrequency;
    t.normalEpsilon        = sh.normalEpsilon;
    t.mountainPower        = sh.mountainPower;
    t.mountainGain         = sh.mountainGain;
    t.mountainSmoothing    = sh.mountainSmoothing;

    const auto& tec = cfg.tectonics;
    t.useTectonics           = tec.enabled;
    t.numPlates              = tec.numPlates;
    t.continentalFraction    = tec.continentalFraction;
    t.boundaryWidth          = tec.boundaryWidth;
    t.convergentMountainScale = tec.convergentMountainScale;
    t.divergentRiftDepth     = tec.divergentRiftDepth;
    t.coastlineNoise         = tec.coastlineNoise;
    t.plateElevationNoise    = tec.plateElevationNoise;

    const auto& ofl = cfg.oceanFloor;
    t.useOceanFloor       = ofl.enabled;
    t.shelfWidth          = ofl.shelfWidth;
    t.oceanRidgeOctaves   = ofl.oceanRidgeOctaves;
    t.oceanRidgeScale     = ofl.oceanRidgeScale;
    t.oceanRidgeStrength  = ofl.oceanRidgeStrength;
    t.oceanRidgePower     = ofl.oceanRidgePower;
    t.oceanRidgeGain      = ofl.oceanRidgeGain;
    t.trenchOctaves       = ofl.trenchOctaves;
    t.trenchScale         = ofl.trenchScale;
    t.trenchDepth         = ofl.trenchDepth;
    t.abyssalOctaves      = ofl.abyssalOctaves;
    t.abyssalScale        = ofl.abyssalScale;
    t.abyssalStrength     = ofl.abyssalStrength;

    const auto& hd = cfg.heightDetail;
    t.detailLowThreshold  = hd.detailLowThreshold;
    t.detailHighThreshold = hd.detailHighThreshold;
    t.perturbStrengthLow  = hd.perturbStrengthLow;
    t.perturbStrengthHigh = hd.perturbStrengthHigh;
    t.detailOctavesLow    = hd.detailOctavesLow;
    t.detailOctavesHigh   = hd.detailOctavesHigh;
    t.detailPersistence   = hd.detailPersistence;
    t.detailLacunarity    = hd.detailLacunarity;
    t.perturbScale        = hd.perturbScale;

    const auto& er = cfg.erosion;
    t.enableErosion      = er.enabled;
    t.erosionIterations  = er.iterations;
    t.thermalErosionRate = er.thermalRate;
    t.thermalThreshold   = er.thermalThreshold;
    t.hydraulicErosionRate = er.hydraulicRate;
    t.depositionRate     = er.depositionRate;
    t.evaporationRate    = er.evaporationRate;

    const auto& sd = cfg.shading;
    s.detailNoiseScale      = sd.detailNoiseScale;
    s.smallNoiseScale       = sd.smallNoiseScale;
    s.smallNoiseOctaves     = sd.smallNoiseOctaves;
    s.warpStrength          = sd.warpStrength;
    s.useClimateModel       = sd.useClimateModel;
    s.largeNoiseScale       = sd.largeNoiseScale;
    s.largeNoiseOctaves     = sd.largeNoiseOctaves;
    s.temperatureLapseRate  = sd.temperatureLapseRate;
    s.temperatureExponent   = sd.temperatureExponent;
    s.moistureNoiseScale    = sd.moistureNoiseScale;
    s.moistureNoiseStrength = sd.moistureNoiseStrength;
    s.hadleyIntensity       = sd.hadleyIntensity;
    s.continentalityStrength = sd.continentalityStrength;
    s.flatColBlend          = sd.flatColBlend;
    s.flatColBlendNoise     = sd.flatColBlendNoise;

    b.enabled           = sd.biomesEnabled;
    b.steepnessThreshold = sd.steepnessThreshold;
    b.flatToSteepBlend  = sd.flatToSteepBlend;
    b.snowLatitude      = sd.snowLatitude;
    b.snowBlend         = sd.snowBlend;
    b.snowLine          = sd.snowLine;
    b.shoreHeight       = sd.shoreHeight;
    b.coastalDepthRange = sd.coastalDepthRange;
    b.aoStrength        = sd.aoStrength;

    c.shoreLow  = sd.colorShoreLow;
    c.shoreHigh = sd.colorShoreHigh;
    c.flatLowA  = sd.colorFlatLowA;
    c.flatHighA = sd.colorFlatHighA;
    c.flatLowB  = sd.colorFlatLowB;
    c.flatHighB = sd.colorFlatHighB;
    c.steepLow  = sd.colorSteepLow;
    c.steepHigh = sd.colorSteepHigh;
    c.snow      = sd.colorSnow;
}

} // namespace

Application::Application()
    : _seaLevel(AppDefaults::SeaLevel)
    , _moveSpeed(AppDefaults::MoveSpeed)
    , _lastTime(0.0f)
    , _previousWidth(0)
    , _previousHeight(0)
{
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    render::WindowConfig windowConfig;
    windowConfig.title = "Procedural Planets";
    if (_capture.enabled)
    {
        // Capture mode forces deterministic size + vsync off so frames render as fast as possible.
        windowConfig.width = _capture.width;
        windowConfig.height = _capture.height;
        windowConfig.vsync = false;
    }
    if (!_window.Initialize(windowConfig))
        return false;

    _input.Initialize(_window.GetHandle());
    _guiManager.Initialize(_window.GetHandle());

    if (!_renderer.Initialize(_window.GetWidth(), _window.GetHeight()))
    {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    // useExistingContext=true: the library shares the app's GL context so per-patch
    // dispatch writes into the app's own LOD buffers.
    try
    {
        _libContext = std::make_unique<pg::Context>(/*useExistingContext=*/true);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Application] FATAL: failed to create libplanetgen context: "
                  << e.what() << std::endl;
        return false;
    }

    // Capture mode: seed must be set before generation so the shot is deterministic.
    if (_capture.enabled)
        _genConfig.seed = _capture.seed;

    // Load earth body from JSON and initialize
    auto earthCfg = LoadBodyConfig("data/bodies/earth.json");
    SyncConfigToEarth(earthCfg, _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);
    SwitchBody(std::move(earthCfg));

    // Initial generation (LOD-only; GPU is a hard requirement)
    RegenerateLodSystem();

    if (_capture.enabled && _capture.hasCamera)
        _camera.SetPosition(_capture.cameraPos);
    else
        _camera.SetPosition(
            glm::vec3(0.0f, 0.0f, _lodConfig.planetRadius * AppDefaults::InitialCameraDistanceMultiplier));

    _lastTime = static_cast<float>(glfwGetTime());
    _previousWidth = _window.GetWidth();
    _previousHeight = _window.GetHeight();

    return true;
}

void Application::Run()
{
    while (!_window.ShouldClose())
    {
        float currentTime = static_cast<float>(glfwGetTime());
        _deltaTime = currentTime - _lastTime;
        _lastTime = currentTime;

        _window.PollEvents();
        _input.Update();

        ProcessInput(_deltaTime);

        // Handle window resize
        int currentWidth = _window.GetWidth();
        int currentHeight = _window.GetHeight();
        if (currentWidth != _previousWidth || currentHeight != _previousHeight)
        {
            _renderer.ResizeFramebuffer(currentWidth, currentHeight);
            _previousWidth = currentWidth;
            _previousHeight = currentHeight;
        }

        Render();
        _window.SwapBuffers();
    }
}

void Application::ApplyCaptureOverrides(const CaptureRequest& request)
{
    _capture = request;
}

bool Application::RunCapture(const CaptureRequest& request)
{
    ApplyCaptureOverrides(request);

    if (!Initialize())
    {
        std::cerr << "[Capture] Initialization failed." << std::endl;
        return false;
    }

    // Warm-up: the LOD scheduler generates patches asynchronously over several frames.
    // Render `frames` frames so the planet is fully populated before the shot.
    int warmup = (std::max)(1, _capture.frames);
    for (int i = 0; i < warmup; ++i)
    {
        float currentTime = static_cast<float>(glfwGetTime());
        _deltaTime = currentTime - _lastTime;
        _lastTime = currentTime;

        _window.PollEvents();
        Render();
        _window.SwapBuffers();
    }

    // Final frame: flag the capture so Render() writes the PNG at the pre-GUI point.
    _headlessCaptureRequested = true;
    Render();
    _window.SwapBuffers();

    bool ok = !_headlessCaptureRequested; // cleared once the capture ran
    if (!ok)
        std::cerr << "[Capture] Capture did not run (render path skipped it)." << std::endl;

    Shutdown();
    return ok;
}

void Application::Shutdown()
{
    _guiManager.Shutdown();
    _renderer.Shutdown();
    _window.Shutdown();
}

void Application::ProcessInput(float deltaTime)
{
    bool guiWantsKeyboard = ImGui::GetIO().WantTextInput;

    if (_input.IsKeyDown(Key::Escape))
    {
        if (_cinematicController.IsPlaying())
            StopCinematic();
        else
            _window.SetShouldClose(true);
    }

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::Tab))
        _guiVisible = !_guiVisible;

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::F5))
    {
        if (_cinematicController.IsPlaying())
            StopCinematic();
        else
            StartCinematic();
    }

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::F12))
        _screenshotRequested = true;

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::G))
    {
        _autoOrbit = !_autoOrbit;
        if (_autoOrbit)
        {
            _camera.SetOrbitTarget(glm::vec3(0.0f));
            _camera.SetMode(core::CameraMode::Orbit);
        }
        else
        {
            _camera.SetMode(core::CameraMode::FreeFly);
        }
    }

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::H))
        _atmosphereSettings.enabled = !_atmosphereSettings.enabled;

    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::R))
        ShuffleTerrain();

    if (_cinematicController.IsPlaying())
    {
        _cinematicController.Update(deltaTime, _camera);
        if (!_cinematicController.IsPlaying())
            StopCinematic();
        return;
    }

    if (_autoOrbit)
    {
        _camera.OrbitRotate(_autoOrbitSpeed * deltaTime, 0.0f);
        return;
    }

    if (_input.IsMouseButtonDown(MouseButton::Right))
    {
        if (_input.IsCursorEnabled())
            _input.SetCursorEnabled(false);
        _camera.Rotate(_input.GetMouseDeltaX() * AppDefaults::MouseRotationSensitivity,
                       _input.GetMouseDeltaY() * AppDefaults::MouseRotationSensitivity);
    }
    else
    {
        if (!_input.IsCursorEnabled())
            _input.SetCursorEnabled(true);
    }

    float speedMultiplier = _input.IsKeyDown(Key::LeftShift) ? AppDefaults::SprintSpeedMultiplier : 1.0f;
    float speed = _moveSpeed * deltaTime * speedMultiplier;

    if (_input.IsKeyDown(Key::W)) _camera.MoveForward(speed);
    if (_input.IsKeyDown(Key::S)) _camera.MoveForward(-speed);
    if (_input.IsKeyDown(Key::A)) _camera.MoveRight(-speed);
    if (_input.IsKeyDown(Key::D)) _camera.MoveRight(speed);
    if (_input.IsKeyDown(Key::E)) _camera.MoveUp(speed);
    if (_input.IsKeyDown(Key::Q)) _camera.MoveUp(-speed);
}

void Application::Render()
{
    // Keep GUI-edited Earth settings reflected in the body config before drawing.
    if (_activeBody && _activeBody->Config().metadata.typeName == "earth")
        SyncEarthToConfig(_activeBody->Config(),
                          _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);

    render::RenderContext ctx;
    ctx.view = _camera.GetViewMatrix();
    ctx.projection = _camera.GetProjectionMatrix(_window.GetAspectRatio());
    ctx.cameraPos = _camera.GetPosition();
    ctx.nearPlane = _camera.GetNearPlane();
    ctx.farPlane = _camera.GetFarPlane();
    ctx.time = _lastTime;
    ctx.scene = &_sceneSettings;
    ctx.ocean = &_oceanSettings;
    ctx.atmosphere = &_atmosphereSettings;
    ctx.biome = &_biomeSettings;
    ctx.body = _activeBody.get();
    ctx.planetRadius = _lodConfig.planetRadius;
    ctx.seaLevel = _seaLevel;

    _renderer.DrawScene(ctx);

    if (_screenshotRequested)
    {
        _captureManager.CaptureScreenshot(_window.GetWidth(), _window.GetHeight());
        _screenshotRequested = false;
    }

    // Headless capture point: scene composited, before the GUI overlay.
    if (_headlessCaptureRequested)
    {
        _captureManager.CaptureScreenshotToFile(_capture.outputPath, _window.GetWidth(), _window.GetHeight());
        _headlessCaptureRequested = false;
    }

    RenderGui();
}

void Application::RenderGui()
{
    _guiManager.BeginFrame();

    if (_guiVisible)
    {
        auto& visibility = _guiManager.GetVisibility();

        // Body type selector
        if (ImGui::Begin("Body Type"))
        {
            const char* bodyTypes[] = {"Earth", "Volcanic World", "Crystalline World"};
            int prevSelected = _selectedBodyType;
            ImGui::Combo("Type", &_selectedBodyType, bodyTypes, IM_ARRAYSIZE(bodyTypes));

            if (_selectedBodyType != prevSelected)
            {
                planetgen::BodyConfig newCfg;
                switch (_selectedBodyType)
                {
                case 0:
                    newCfg = LoadBodyConfig("data/bodies/earth.json");
                    SyncConfigToEarth(newCfg, _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);
                    break;
                case 1:
                    newCfg = LoadBodyConfig("data/bodies/volcanic.json");
                    break;
                case 2:
                    newCfg = LoadBodyConfig("data/bodies/crystalline.json");
                    break;
                }
                SwitchBody(std::move(newCfg));
            }

            if (_activeBody)
                ImGui::Text("Active: %s", _activeBody->GetTypeName().c_str());
        }
        ImGui::End();

        _scenePanel.Draw(_sceneSettings, visibility.scene);

        _terrainStats.patchCount = _renderer.QuadTree().GetActiveLeafCount();
        _terrainStats.visiblePatchCount = _renderer.QuadTree().GetVisiblePatchCount();
        _terrainStats.vertexCount = 0;

        bool randomize = false;
        bool needsRegen =
            _terrainPanel.Draw(_genConfig, _terrainSettings, _lodConfig, _terrainStats, visibility.terrain, randomize);

        if (randomize)
        {
            render::RandomizeEarthParameters(_terrainSettings, _shadingSettings, _biomeSettings,
                                             _sceneSettings, _atmosphereSettings, _oceanSettings, _genConfig.seed);
            needsRegen = true;
        }

        needsRegen |= _surfacePanel.Draw(_activeBody.get(), _oceanSettings, _seaLevel, visibility.surface);
        _atmospherePanel.Draw(_atmosphereSettings, visibility.atmosphere);
        _debugPanel.Draw(_camera, _moveSpeed, _autoOrbit, _autoOrbitSpeed, visibility.debug);

        bool playRequested = false;
        _cinematicPanel.Draw(_cinematicSettings, _lodConfig.planetRadius, playRequested, _cinematicPanelVisible);
        if (playRequested)
            StartCinematic();

        if (needsRegen)
            RegenerateLodSystem();
    }

    _guiManager.EndFrame();
}

void Application::SwitchBody(planetgen::BodyConfig config)
{
    _activeBody = std::make_unique<render::BodyRuntime>(std::move(config), _paletteRegistry);

    // The library owns all terrain compute; hand it the full config as data so it
    // bakes the continent mask from the in-memory (GUI-edited) config.
    {
        const std::string configJson = planetgen::BodyConfigToJson(_activeBody->Config());
        _libBody = _libContext->CreateBodyFromJsonString(configJson);
        if (!_libBody.IsValid())
        {
            std::cerr << "[SwitchBody] FATAL: libplanetgen failed to create body for "
                      << _activeBody->GetTypeName() << ": " << _libContext->GetErrorMessage()
                      << " — aborting." << std::endl;
            std::abort();
        }
    }
    _terrainStats.gpuAvailable = true;

    _lodConfig.planetRadius = _activeBody->GetRadius();
    _seaLevel = _activeBody->GetSeaLevel();
    _atmosphereSettings.enabled = _activeBody->HasAtmosphere();

    RegenerateLodSystem();

    float camDist = _lodConfig.planetRadius * AppDefaults::InitialCameraDistanceMultiplier;
    _camera.SetPosition(glm::vec3(0.0f, 0.0f, camDist));
    float farPlane = (std::max)(AppDefaults::MinFarPlane,
                                _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);

    std::cout << "[SwitchBody] Switched to " << _activeBody->GetTypeName()
              << " (radius=" << _lodConfig.planetRadius << ")" << std::endl;
}

void Application::ShuffleTerrain()
{
    render::RandomizeEarthParameters(_terrainSettings, _shadingSettings, _biomeSettings,
                                     _sceneSettings, _atmosphereSettings, _oceanSettings, _genConfig.seed);
    RegenerateLodSystem();
}

void Application::StartCinematic()
{
    _camera.SetOrbitTarget(glm::vec3(0.0f));
    _camera.SetMode(core::CameraMode::Orbit);
    _cinematicController.Start(_cinematicSettings, _lodConfig.planetRadius, _camera);
    _guiVisible = false;
}

void Application::StopCinematic()
{
    _cinematicController.Stop();
    _guiVisible = true;
    _camera.SetMode(core::CameraMode::FreeFly);
}

void Application::RegenerateLodSystem()
{
    if (!_activeBody || !_libBody.IsValid())
    {
        std::cerr << "[LOD] No active body — cannot build the LOD system" << std::endl;
        return;
    }

    if (_activeBody->Config().metadata.typeName == "earth")
        SyncEarthToConfig(_activeBody->Config(),
                          _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);

    // Recreate the library body so it re-bakes the continent mask from the current
    // (possibly GUI-edited) config.
    if (_libContext)
    {
        const std::string configJson = planetgen::BodyConfigToJson(_activeBody->Config());
        pg::Body rebuilt = _libContext->CreateBodyFromJsonString(configJson);
        if (rebuilt.IsValid())
            _libBody = std::move(rebuilt);
    }

    _renderer.SetActiveBody(_libBody.Handle(), *_activeBody, BuildQuadTreeConfig(), _seaLevel, _genConfig.seed);

    float farPlane = (std::max)(AppDefaults::MinFarPlane,
                                _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);
}

render::lod::QuadTreeConfig Application::BuildQuadTreeConfig() const
{
    render::lod::QuadTreeConfig config;
    config.planetRadius    = _lodConfig.planetRadius;
    config.baseSubdivisions = _lodConfig.patchSubdivisions;
    config.meshResolution  = _lodConfig.meshResolution;
    config.maxDepth        = _lodConfig.maxDepth;
    config.splitThreshold  = _lodConfig.splitThreshold;
    config.hysteresis      = _lodConfig.hysteresis;
    config.maxActivePatches = _lodConfig.maxActivePatches;
    config.skirtFraction   = _lodConfig.skirtFraction;
    return config;
}

} // namespace planets::app
