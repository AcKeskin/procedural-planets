#include "Application.h"
#include "GpuConstants.h"
#include "MeshGenerator.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <iostream>
#include <chrono>
#include <random>

namespace planets::app
{

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

    if (!_planetShader.LoadFromFiles("shaders/earth/planet_earth.vert", "shaders/earth/planet_earth.frag"))
    {
        std::cerr << "Failed to load planet shader" << std::endl;
        return false;
    }

    if (!_spaceShader.LoadFromFiles("shaders/space.vert", "shaders/space.frag"))
    {
        std::cerr << "Failed to load space shader" << std::endl;
        return false;
    }

    // GPU terrain generator
    _genConfig.useGpu = _terrainGenerator.Initialize("shaders/compute/height_earth.comp",
                                                     "shaders/compute/shading_earth.comp",
                                                     "shaders/compute/erosion_earth.comp");
    _terrainStats.gpuAvailable = _genConfig.useGpu;

    if (_genConfig.useGpu)
        std::cout << "[TerrainGenerator] GPU compute shaders loaded successfully" << std::endl;
    else
        std::cout << "[TerrainGenerator] GPU compute shaders not available, using CPU" << std::endl;

    // Initialize async generation scheduler
    if (_genConfig.useGpu)
    {
        SyncEarthSettings();
        _generationScheduler.Initialize(_terrainGenerator, _earth, _genConfig.seed);
    }

    // Load biome palette
    _biomePalette = render::BiomePalette::LoadFromJson("data/palettes/earth.json");
    if (_biomePalette.IsValid())
        _biomePalette.UploadToGpu(_paletteBuffer);

    // CPU fallback planet
    core::PlanetSettings planetSettings;
    planetSettings.radius = _lodConfig.planetRadius;
    planetSettings.seaLevel = _seaLevel;
    planetSettings.subdivisions = _genConfig.subdivisions;
    planetSettings.seed = _genConfig.seed;
    _planet.Configure(planetSettings);

    // Initial generation
    if (_lodConfig.enabled && _genConfig.useGpu)
        RegenerateLodSystem();
    else if (_genConfig.useGpu)
        RegeneratePlanetGpu();
    else
        RegeneratePlanetCpu();

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
    // Suppress keybinds only while an ImGui text input is active
    bool guiWantsKeyboard = ImGui::GetIO().WantTextInput;

    // ESC: context-aware (stop cinematic or exit)
    if (_input.IsKeyDown(Key::Escape))
    {
        if (_cinematicController.IsPlaying())
            StopCinematic();
        else
            _window.SetShouldClose(true);
    }

    // Tab: toggle GUI visibility
    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::Tab))
        _guiVisible = !_guiVisible;

    // F5: toggle cinematic playback
    if (!guiWantsKeyboard && _input.IsKeyPressed(Key::F5))
    {
        if (_cinematicController.IsPlaying())
            StopCinematic();
        else
            StartCinematic();
    }

    // F12: screenshot (deferred to capture point in Render)
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

    // Cinematic update: takes priority over all camera controls
    if (_cinematicController.IsPlaying())
    {
        _cinematicController.Update(deltaTime, _camera);

        // Auto-stop check: controller sets state to Idle when done
        if (!_cinematicController.IsPlaying())
            StopCinematic();
        return;
    }

    // Auto-orbit: rotate around planet, skip manual controls
    if (_autoOrbit)
    {
        _camera.OrbitRotate(_autoOrbitSpeed * deltaTime, 0.0f);
        return;
    }

    // Manual camera controls
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

    if (_input.IsKeyDown(Key::W))
        _camera.MoveForward(speed);
    if (_input.IsKeyDown(Key::S))
        _camera.MoveForward(-speed);
    if (_input.IsKeyDown(Key::A))
        _camera.MoveRight(-speed);
    if (_input.IsKeyDown(Key::D))
        _camera.MoveRight(speed);
    if (_input.IsKeyDown(Key::E))
        _camera.MoveUp(speed);
    if (_input.IsKeyDown(Key::Q))
        _camera.MoveUp(-speed);
}

void Application::Render()
{
    // Phase 1: Render scene to framebuffer
    _sceneFbo.Bind();
    _renderer.BeginFrame();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = _camera.GetViewMatrix();
    glm::mat4 projection = _camera.GetProjectionMatrix(_window.GetAspectRatio());
    glm::mat4 invView = glm::inverse(view);
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

    // Biome uniforms
    _planetShader.SetInt("uUseBiomes", _biomeSettings.enabled ? 1 : 0);
    _planetShader.SetFloat("uSteepnessThreshold", _biomeSettings.steepnessThreshold);
    _planetShader.SetFloat("uFlatToSteepBlend", _biomeSettings.flatToSteepBlend);
    _planetShader.SetFloat("uSnowLatitude", _biomeSettings.snowLatitude);
    _planetShader.SetFloat("uSnowBlend", _biomeSettings.snowBlend);
    _planetShader.SetFloat("uSnowLine", _biomeSettings.snowLine);
    _planetShader.SetFloat("uShoreHeight", _biomeSettings.shoreHeight);
    _planetShader.SetInt("uUseClimateModel", _shadingSettings.useClimateModel ? 1 : 0);

    // Earth colors
    _planetShader.SetVec3("uShoreLow", _earthColors.shoreLow);
    _planetShader.SetVec3("uShoreHigh", _earthColors.shoreHigh);
    _planetShader.SetVec3("uFlatLowA", _earthColors.flatLowA);
    _planetShader.SetVec3("uFlatHighA", _earthColors.flatHighA);
    _planetShader.SetVec3("uFlatLowB", _earthColors.flatLowB);
    _planetShader.SetVec3("uFlatHighB", _earthColors.flatHighB);
    _planetShader.SetVec3("uSteepLow", _earthColors.steepLow);
    _planetShader.SetVec3("uSteepHigh", _earthColors.steepHigh);
    _planetShader.SetVec3("uSnowColor", _earthColors.snow);

    // Shading parameters
    _planetShader.SetFloat("uFlatColBlend", _shadingSettings.flatColBlend);
    _planetShader.SetFloat("uFlatColBlendNoise", _shadingSettings.flatColBlendNoise);

    // Height range (auto-computed from terrain scale)
    float heightMin = 1.0f - _terrainSettings.heightScale * AppDefaults::HeightRangeMultiplier;
    float heightMax = 1.0f + _terrainSettings.heightScale * AppDefaults::HeightRangeMultiplier;
    _planetShader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

    // Lighting
    _planetShader.SetFloat("uSunIntensity", _sceneSettings.lighting.sunIntensity);
    _planetShader.SetFloat("uAmbientLight", _sceneSettings.lighting.ambientLight);
    _planetShader.SetFloat("uSpecularStrength", _sceneSettings.lighting.specularStrength);
    _planetShader.SetFloat("uSpecularPower", _sceneSettings.lighting.specularPower);

    // Fresnel rim for sense of scale
    _planetShader.SetFloat("uPlanetScale", _lodConfig.planetRadius);
    _planetShader.SetFloat("uFresnelStrengthNear", AppDefaults::FresnelStrengthNear);
    _planetShader.SetFloat("uFresnelStrengthFar", AppDefaults::FresnelStrengthFar);
    _planetShader.SetFloat("uFresnelPow", AppDefaults::FresnelPower);

    // Visual quality: detail fade, coastal gradient, AO, atmospheric perspective
    _planetShader.SetFloat("uDetailFadeStart", _sceneSettings.lighting.detailFadeStart);
    _planetShader.SetFloat("uCoastalDepthRange", _biomeSettings.coastalDepthRange);
    _planetShader.SetFloat("uAOStrength", _biomeSettings.aoStrength);
    _planetShader.SetFloat("uHazeStrength", _sceneSettings.lighting.hazeStrength);
    _planetShader.SetVec3("uHazeColor", _sceneSettings.lighting.hazeColor);

    // Biome palette SSBO
    if (_paletteBuffer.IsValid())
    {
        _paletteBuffer.Bind(render::BiomePaletteBindingPoint);
        _planetShader.SetInt("uPaletteSize", _biomePalette.GetEntryCount());
    }

    if (_lodConfig.enabled && _genConfig.useGpu)
    {
        // Async pipeline: process scheduler, apply results, then update + render
        _generationScheduler.ProcessFrame(AppDefaults::SchedulerPatchesPerFrame, AppDefaults::SchedulerPatchesPerFrame);
        _quadTree.ApplyCompletedPatches();
        _quadTree.Update(_camera.GetPosition(), viewProjection);
        _quadTree.Render(_planetShader);
        _oceanRenderer.Render(
            view, projection, _camera.GetPosition(), _sceneSettings.lightDir, _lastTime, _oceanSettings);
    }
    else
    {
        _planetMesh.Draw();
    }

    _sceneFbo.Unbind();

    // Phase 2: Atmosphere post-process
    _renderer.BeginFrame();

    float oceanRadius = _lodConfig.planetRadius * _seaLevel;
    glm::vec3 planetCenter = glm::vec3(0.0f);

    if (_atmosphereSettings.enabled)
    {
        _atmosphereRenderer.Render(view,
                                   projection,
                                   invView,
                                   invProjection,
                                   _camera.GetPosition(),
                                   _sceneSettings.lightDir,
                                   planetCenter,
                                   _lodConfig.planetRadius,
                                   oceanRadius,
                                   _camera.GetNearPlane(),
                                   _camera.GetFarPlane(),
                                   _atmosphereSettings,
                                   _sceneFbo.GetColorTexture(),
                                   _sceneFbo.GetDepthTexture());
    }
    else
    {
        _passthroughShader.Use();
        _passthroughShader.SetInt("uSceneTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _sceneFbo.GetColorTexture());
        _postProcessor.RenderQuad();
    }

    // Capture point: scene fully rendered, before GUI overlay
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

        _scenePanel.Draw(_sceneSettings, visibility.scene);

        // Update terrain stats before drawing panel
        _terrainStats.patchCount = _quadTree.GetActiveLeafCount();
        _terrainStats.visiblePatchCount = _quadTree.GetVisiblePatchCount();
        _terrainStats.vertexCount = 0;

        bool randomize = false;
        bool needsRegen = _terrainPanel.Draw(
            _genConfig, _terrainSettings, _lodConfig, _terrainStats, _planet, visibility.terrain, randomize);

        if (randomize)
        {
            render::RandomizeEarthParameters(_terrainSettings,
                                             _shadingSettings,
                                             _biomeSettings,
                                             _sceneSettings,
                                             _atmosphereSettings,
                                             _oceanSettings,
                                             _genConfig.seed);
            needsRegen = true;
        }

        // Sync CPU planet settings back to generation config
        if (!_genConfig.useGpu)
        {
            _genConfig.subdivisions = _planet.GetSettings().subdivisions;
            _genConfig.seed = _planet.GetSettings().seed;
        }

        needsRegen |= _surfacePanel.Draw(
            _biomeSettings, _earthColors, _shadingSettings, _oceanSettings, _seaLevel, visibility.surface);
        _atmospherePanel.Draw(_atmosphereSettings, visibility.atmosphere);
        _debugPanel.Draw(_camera, _moveSpeed, _autoOrbit, _autoOrbitSpeed, visibility.debug);

        // Cinematic panel: only when not playing
        bool playRequested = false;
        _cinematicPanel.Draw(_cinematicSettings, _lodConfig.planetRadius, playRequested, _cinematicPanelVisible);

        if (playRequested)
            StartCinematic();

        if (needsRegen)
        {
            if (_lodConfig.enabled && _genConfig.useGpu)
                RegenerateLodSystem();
            else if (_genConfig.useGpu)
                RegeneratePlanetGpu();
            else
                RegeneratePlanetCpu();
        }
    }

    _guiManager.EndFrame();
}

void Application::SyncEarthSettings()
{
    _earth.GetTerrainSettings() = _terrainSettings;
    _earth.GetShadingSettings() = _shadingSettings;
    _earth.GetBiomeSettings() = _biomeSettings;
    _earth.GetColors() = _earthColors;
}

void Application::ShuffleTerrain()
{
    render::RandomizeEarthParameters(_terrainSettings,
                                     _shadingSettings,
                                     _biomeSettings,
                                     _sceneSettings,
                                     _atmosphereSettings,
                                     _oceanSettings,
                                     _genConfig.seed);

    if (_lodConfig.enabled && _genConfig.useGpu)
        RegenerateLodSystem();
    else if (_genConfig.useGpu)
        RegeneratePlanetGpu();
    else
        RegeneratePlanetCpu();
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

void Application::RegeneratePlanetCpu()
{
    auto start = std::chrono::high_resolution_clock::now();

    core::PlanetSettings planetSettings;
    planetSettings.subdivisions = _genConfig.subdivisions;
    planetSettings.seed = _genConfig.seed;
    _planet.Configure(planetSettings);
    _planet.Reseed(_genConfig.seed);

    auto meshData = render::MeshGenerator::GeneratePlanetMesh(_planet, _genConfig.subdivisions);
    _planetMesh.Upload(meshData);

    auto end = std::chrono::high_resolution_clock::now();
    _terrainStats.cpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    std::cout << "[Planet] CPU generated " << _planetMesh.GetVertexCount() << " vertices in " << _terrainStats.cpuTimeMs
              << " ms" << std::endl;
}

void Application::RegeneratePlanetGpu()
{
    if (!_terrainGenerator.IsReady())
    {
        RegeneratePlanetCpu();
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    auto vertices = render::MeshGenerator::GetIcosphereVertices(_genConfig.subdivisions);
    auto heights = _terrainGenerator.GenerateHeights(vertices, _genConfig.seed, _terrainSettings);

    render::MeshData meshData;
    if (_terrainGenerator.IsShadingReady())
    {
        auto shadingData = _terrainGenerator.GenerateShadingData(vertices, _genConfig.seed, _shadingSettings);
        meshData = render::MeshGenerator::GeneratePlanetMesh(
            _genConfig.subdivisions, _lodConfig.planetRadius, heights, shadingData);
    }
    else
    {
        meshData = render::MeshGenerator::GeneratePlanetMesh(_genConfig.subdivisions, _lodConfig.planetRadius, heights);
    }

    _planetMesh.Upload(meshData);

    auto end = std::chrono::high_resolution_clock::now();
    _terrainStats.gpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    std::cout << "[Planet] GPU generated " << _planetMesh.GetVertexCount() << " vertices in " << _terrainStats.gpuTimeMs
              << " ms" << std::endl;
}

void Application::RegenerateLodSystem()
{
    if (!_terrainGenerator.IsReady())
    {
        std::cerr << "[LOD] GPU compute shader required for LOD system" << std::endl;
        return;
    }

    // Cancel all in-flight async work before rebuilding
    _generationScheduler.CancelAll();
    SyncEarthSettings();
    _generationScheduler.SetBody(_earth, _genConfig.seed);

    auto qtConfig = BuildQuadTreeConfig();

    // Build tree structure and enqueue patches (non-blocking)
    _quadTree.Initialize(qtConfig, _generationScheduler, _genConfig.seed);
    _oceanRenderer.Initialize(_lodConfig.planetRadius, _seaLevel, AppDefaults::OceanSubdivisions);
    _atmosphereRenderer.Initialize();

    float farPlane =
        (std::max)(AppDefaults::MinFarPlane, _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);

    std::cout << "[QuadTree] Enqueued " << _generationScheduler.GetPendingCount() << " patches for async generation"
              << std::endl;
}

render::lod::QuadTreeConfig Application::BuildQuadTreeConfig() const
{
    render::lod::QuadTreeConfig config;
    config.planetRadius = _lodConfig.planetRadius;
    config.baseSubdivisions = _lodConfig.patchSubdivisions;
    config.meshResolution = _lodConfig.meshResolution;
    config.maxDepth = _lodConfig.maxDepth;
    config.splitThreshold = _lodConfig.splitThreshold;
    config.hysteresis = _lodConfig.hysteresis;
    config.maxActivePatches = _lodConfig.maxActivePatches;
    config.skirtFraction = _lodConfig.skirtFraction;
    return config;
}

} // namespace planets::app
