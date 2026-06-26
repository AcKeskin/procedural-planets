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
    sh.heightScale          = t.heightScale;
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

    auto& sd = cfg.shading;
    sd.detailNoiseScale      = s.detailNoiseScale;
    sd.smallNoiseScale       = s.smallNoiseScale;
    sd.smallNoiseOctaves     = s.smallNoiseOctaves;
    sd.warpStrength          = s.warpStrength;
    sd.useClimateModel       = s.useClimateModel;
    sd.largeNoiseScale       = s.largeNoiseScale;
    sd.largeNoiseOctaves     = s.largeNoiseOctaves;
    sd.temperatureLapseRate  = s.temperatureLapseRate;
    sd.temperatureExponent   = s.temperatureExponent;
    sd.moistureNoiseScale    = s.moistureNoiseScale;
    sd.moistureNoiseStrength = s.moistureNoiseStrength;
    sd.hadleyIntensity       = s.hadleyIntensity;
    sd.continentalityStrength = s.continentalityStrength;
    sd.flatColBlend          = s.flatColBlend;
    sd.flatColBlendNoise     = s.flatColBlendNoise;
    sd.biomesEnabled         = b.enabled;
    sd.steepnessThreshold    = b.steepnessThreshold;
    sd.flatToSteepBlend      = b.flatToSteepBlend;
    sd.snowLatitude          = b.snowLatitude;
    sd.snowBlend             = b.snowBlend;
    sd.snowLine              = b.snowLine;
    sd.shoreHeight           = b.shoreHeight;
    sd.coastalDepthRange     = b.coastalDepthRange;
    sd.aoStrength            = b.aoStrength;
    sd.colorShoreLow         = c.shoreLow;
    sd.colorShoreHigh        = c.shoreHigh;
    sd.colorFlatLowA         = c.flatLowA;
    sd.colorFlatHighA        = c.flatHighA;
    sd.colorFlatLowB         = c.flatLowB;
    sd.colorFlatHighB        = c.flatHighB;
    sd.colorSteepLow         = c.steepLow;
    sd.colorSteepHigh        = c.steepHigh;
    sd.colorSnow             = c.snow;
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
    t.heightScale          = sh.heightScale;
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
    if (!_window.Initialize(windowConfig))
        return false;

    _input.Initialize(_window.GetHandle());
    _renderer.Initialize();
    _guiManager.Initialize(_window.GetHandle());

    // Initialize libplanetgen with the app's existing GL context
    _terrainGenerator.InitializeLibrary();

    if (!_sceneFbo.Create(_window.GetWidth(), _window.GetHeight()))
    {
        std::cerr << "Failed to create scene framebuffer" << std::endl;
        return false;
    }

    if (!_postProcessor.Initialize())
    {
        std::cerr << "Failed to initialize post-processor" << std::endl;
        return false;
    }

    if (!_passthroughShader.LoadFromFiles("shaders/passthrough.vert", "shaders/passthrough.frag"))
    {
        std::cerr << "Failed to load passthrough shader" << std::endl;
        return false;
    }

    if (!_spaceShader.LoadFromFiles("shaders/space.vert", "shaders/space.frag"))
    {
        std::cerr << "Failed to load space shader" << std::endl;
        return false;
    }

    // Load earth body from JSON and initialize
    auto earthCfg = LoadBodyConfig("data/bodies/earth.json");
    SyncConfigToEarth(earthCfg, _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);
    SwitchBody(std::move(earthCfg));

    // Initial generation (LOD-only; GPU is a hard requirement)
    RegenerateLodSystem();

    _camera.SetPosition(glm::vec3(0.0f, 0.0f, _lodConfig.planetRadius * AppDefaults::InitialCameraDistanceMultiplier));

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
            _sceneFbo.Resize(currentWidth, currentHeight);
            _previousWidth = currentWidth;
            _previousHeight = currentHeight;
        }

        Render();

        _renderer.EndFrame();
        _window.SwapBuffers();
    }
}

void Application::Shutdown()
{
    _generationScheduler.CancelAll();
    _postProcessor.Shutdown();
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
    _sceneFbo.Bind();
    _renderer.BeginFrame();

    glm::mat4 model      = glm::mat4(1.0f);
    glm::mat4 view       = _camera.GetViewMatrix();
    glm::mat4 projection = _camera.GetProjectionMatrix(_window.GetAspectRatio());
    glm::mat4 invView    = glm::inverse(view);
    glm::mat4 invProjection = glm::inverse(projection);
    glm::mat4 viewProjection = projection * view;

    // Space background
    glDepthMask(GL_FALSE);
    _spaceShader.Use();
    _spaceShader.SetMat4("uInvProjection", invProjection);
    _spaceShader.SetMat4("uInvView", invView);
    _spaceShader.SetVec3("uLightDir", _sceneSettings.lightDir);
    _spaceShader.SetFloat("uSunSize", _sceneSettings.sunSize);
    _spaceShader.SetVec3("uSunColor", _sceneSettings.sunColor);
    _spaceShader.SetFloat("uStarDensity", _sceneSettings.starDensity);
    _postProcessor.RenderQuad();
    glDepthMask(GL_TRUE);

    // Planet
    _planetShader.Use();
    _planetShader.SetMat4("uModel", model);
    _planetShader.SetMat4("uView", view);
    _planetShader.SetMat4("uProjection", projection);
    _planetShader.SetVec3("uLightDir", _sceneSettings.lightDir);
    _planetShader.SetVec3("uCameraPos", _camera.GetPosition());
    _planetShader.SetFloat("uSeaLevel", _seaLevel);

    // Sync GUI-edited Earth settings into the active body config before rendering
    if (_activeBody && _activeBody->Config().metadata.typeName == "earth")
        SyncEarthToConfig(_activeBody->Config(),
                          _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);

    if (_activeBody)
        _activeBody->SetRenderUniforms(_planetShader);

    _planetShader.SetFloat("uSunIntensity", _sceneSettings.lighting.sunIntensity);
    _planetShader.SetFloat("uAmbientLight", _sceneSettings.lighting.ambientLight);
    _planetShader.SetFloat("uSpecularStrength", _sceneSettings.lighting.specularStrength);
    _planetShader.SetFloat("uSpecularPower", _sceneSettings.lighting.specularPower);

    _planetShader.SetFloat("uPlanetScale", _lodConfig.planetRadius);
    _planetShader.SetFloat("uFresnelStrengthNear", AppDefaults::FresnelStrengthNear);
    _planetShader.SetFloat("uFresnelStrengthFar", AppDefaults::FresnelStrengthFar);
    _planetShader.SetFloat("uFresnelPow", AppDefaults::FresnelPower);

    _planetShader.SetFloat("uDetailFadeStart", _sceneSettings.lighting.detailFadeStart);
    _planetShader.SetFloat("uAOStrength", _biomeSettings.aoStrength);
    _planetShader.SetFloat("uHazeStrength", _sceneSettings.lighting.hazeStrength);
    _planetShader.SetVec3("uHazeColor", _sceneSettings.lighting.hazeColor);

    if (_paletteBuffer.IsValid())
    {
        _paletteBuffer.Bind(render::BiomePaletteBindingPoint);
        _planetShader.SetInt("uPaletteSize", _biomePalette.GetEntryCount());
    }

    _generationScheduler.ProcessFrame(AppDefaults::SchedulerPatchesPerFrame, AppDefaults::SchedulerPatchesPerFrame);
    _quadTree.ApplyCompletedPatches();
    _quadTree.Update(_camera.GetPosition(), viewProjection);
    _quadTree.Render(_planetShader);

    if (_activeBody && _seaLevel > 0.0f)
    {
        _oceanRenderer.Render(
            view, projection, _camera.GetPosition(), _sceneSettings.lightDir, _lastTime, _oceanSettings);
    }

    _sceneFbo.Unbind();

    // Atmosphere post-process
    _renderer.BeginFrame();

    float oceanRadius = _lodConfig.planetRadius * _seaLevel;
    glm::vec3 planetCenter = glm::vec3(0.0f);

    if (_atmosphereSettings.enabled)
    {
        _atmosphereRenderer.Render(view, projection, invView, invProjection,
                                   _camera.GetPosition(), _sceneSettings.lightDir,
                                   planetCenter, _lodConfig.planetRadius, oceanRadius,
                                   _camera.GetNearPlane(), _camera.GetFarPlane(),
                                   _atmosphereSettings,
                                   _sceneFbo.GetColorTexture(), _sceneFbo.GetDepthTexture());
    }
    else
    {
        _passthroughShader.Use();
        _passthroughShader.SetInt("uSceneTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _sceneFbo.GetColorTexture());
        _postProcessor.RenderQuad();
    }

    if (_screenshotRequested)
    {
        _captureManager.CaptureScreenshot(_window.GetWidth(), _window.GetHeight());
        _screenshotRequested = false;
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

        _terrainStats.patchCount = _quadTree.GetActiveLeafCount();
        _terrainStats.visiblePatchCount = _quadTree.GetVisiblePatchCount();
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

        needsRegen |= _surfacePanel.Draw(_activeBody.get(), _biomeSettings, _earthColors,
                                         _shadingSettings, _oceanSettings, _seaLevel, visibility.surface);
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

    // Load body-specific shaders
    if (!_planetShader.LoadFromFiles(_activeBody->GetVertexShaderPath(), _activeBody->GetFragmentShaderPath()))
    {
        std::cerr << "[SwitchBody] Failed to load planet shader for " << _activeBody->GetTypeName() << std::endl;
        return;
    }

    std::string erosionPath = _activeBody->GetErosionShaderPath();
    if (erosionPath.empty())
        erosionPath = "shaders/compute/erosion_earth.comp";

    bool gpuReady = _terrainGenerator.Initialize(
        _activeBody->GetHeightShaderPath(), _activeBody->GetShadingShaderPath(), erosionPath);
    if (!gpuReady)
    {
        std::cerr << "[SwitchBody] FATAL: GPU terrain generator failed to initialize for "
                  << _activeBody->GetTypeName() << " — aborting." << std::endl;
        std::abort();
    }
    _terrainStats.gpuAvailable = true;

    _biomePalette = _activeBody->LoadBiomePalette();
    if (_biomePalette.IsValid())
        _biomePalette.UploadToGpu(_paletteBuffer);

    _lodConfig.planetRadius = _activeBody->GetRadius();
    _seaLevel = _activeBody->GetSeaLevel();
    _atmosphereSettings.enabled = _activeBody->HasAtmosphere();

    _generationScheduler.Initialize(_terrainGenerator, *_activeBody, _genConfig.seed);
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
    if (!_terrainGenerator.IsReady())
    {
        std::cerr << "[LOD] GPU compute shader required for LOD system" << std::endl;
        return;
    }

    _generationScheduler.CancelAll();

    // Sync earth settings if active body is earth type
    if (_activeBody && _activeBody->Config().metadata.typeName == "earth")
        SyncEarthToConfig(_activeBody->Config(),
                          _terrainSettings, _shadingSettings, _biomeSettings, _earthColors);

    _generationScheduler.SetBody(*_activeBody, _genConfig.seed);

    auto qtConfig = BuildQuadTreeConfig();
    _quadTree.Initialize(qtConfig, _generationScheduler, _genConfig.seed);
    _oceanRenderer.Initialize(_lodConfig.planetRadius, _seaLevel, AppDefaults::OceanSubdivisions);
    _atmosphereRenderer.Initialize();

    float farPlane = (std::max)(AppDefaults::MinFarPlane,
                                _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);

    std::cout << "[QuadTree] Enqueued " << _generationScheduler.GetPendingCount()
              << " patches for async generation" << std::endl;
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
