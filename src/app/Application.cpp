#include "Application.h"
#include "MeshGenerator.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

namespace planets::app {

Application::Application()
    : _seaLevel(0.995f)
    , _moveSpeed(15.0f)
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

    if (!_planetShader.LoadFromFiles("shaders/planet.vert", "shaders/planet.frag"))
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
    _genConfig.useGpu = _terrainGenerator.Initialize(
        "shaders/compute/height_earth.comp",
        "shaders/compute/shading_earth.comp");
    _terrainStats.gpuAvailable = _genConfig.useGpu;

    if (_genConfig.useGpu)
        std::cout << "[TerrainGenerator] GPU compute shaders loaded successfully" << std::endl;
    else
        std::cout << "[TerrainGenerator] GPU compute shaders not available, using CPU" << std::endl;

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

    _camera.SetPosition(glm::vec3(0.0f, 0.0f, 25.0f));

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
        float deltaTime = currentTime - _lastTime;
        _lastTime = currentTime;

        _window.PollEvents();
        _input.Update();

        ProcessInput(deltaTime);

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
    _postProcessor.Shutdown();
    _guiManager.Shutdown();
    _renderer.Shutdown();
    _window.Shutdown();
}

void Application::ProcessInput(float deltaTime)
{
    if (_input.IsKeyDown(Key::Escape))
        _window.SetShouldClose(true);

    if (_input.IsMouseButtonDown(MouseButton::Right))
    {
        if (_input.IsCursorEnabled())
            _input.SetCursorEnabled(false);

        _camera.Rotate(_input.GetMouseDeltaX() * 0.1f, _input.GetMouseDeltaY() * 0.1f);
    }
    else
    {
        if (!_input.IsCursorEnabled())
            _input.SetCursorEnabled(true);
    }

    float speedMultiplier = _input.IsKeyDown(Key::LeftShift) ? 5.0f : 1.0f;
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

    // Height range
    float heightMin, heightMax;
    if (_biomeSettings.autoHeightRange)
    {
        heightMin = 1.0f - _terrainSettings.heightScale * 1.5f;
        heightMax = 1.0f + _terrainSettings.heightScale * 1.5f;
    }
    else
    {
        heightMin = _biomeSettings.heightRangeMin;
        heightMax = _biomeSettings.heightRangeMax;
    }
    _planetShader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

    // Lighting
    _planetShader.SetFloat("uSunIntensity", _sceneSettings.lighting.sunIntensity);
    _planetShader.SetFloat("uAmbientLight", _sceneSettings.lighting.ambientLight);
    _planetShader.SetFloat("uSpecularStrength", _sceneSettings.lighting.specularStrength);
    _planetShader.SetFloat("uSpecularPower", _sceneSettings.lighting.specularPower);

    if (_lodConfig.enabled && _genConfig.useGpu)
    {
        _lodSystem.UpdateLods(_camera.GetPosition(), viewProjection);
        _lodSystem.Render(_planetShader);
        _oceanRenderer.Render(view, projection, _camera.GetPosition(),
                              _sceneSettings.lightDir, _oceanSettings);
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
        _atmosphereRenderer.Render(
            view, projection, invView, invProjection,
            _camera.GetPosition(), _sceneSettings.lightDir,
            planetCenter, _lodConfig.planetRadius, oceanRadius,
            0.1f, 1000.0f,
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

    RenderGui();
}

void Application::RenderGui()
{
    _guiManager.BeginFrame();

    auto& visibility = _guiManager.GetVisibility();

    _scenePanel.Draw(_sceneSettings, visibility.scene);

    // Update terrain stats before drawing panel
    _terrainStats.patchCount = _lodSystem.GetPatchCount();
    _terrainStats.visiblePatchCount = _lodSystem.GetVisiblePatchCount();
    _terrainStats.vertexCount = _lodSystem.GetTotalVertexCount();

    bool needsRegen = _terrainPanel.Draw(
        _genConfig, _terrainSettings, _lodConfig, _terrainStats,
        _planet, visibility.terrain);

    // Sync CPU planet settings back to generation config
    if (!_genConfig.useGpu)
    {
        _genConfig.subdivisions = _planet.GetSettings().subdivisions;
        _genConfig.seed = _planet.GetSettings().seed;
    }

    _surfacePanel.Draw(_biomeSettings, _earthColors, _shadingSettings,
                       _oceanSettings, _seaLevel, visibility.surface);
    _atmospherePanel.Draw(_atmosphereSettings, visibility.atmosphere);
    _debugPanel.Draw(_camera, _moveSpeed, visibility.debug);

    if (needsRegen)
    {
        if (_lodConfig.enabled && _genConfig.useGpu)
            RegenerateLodSystem();
        else if (_genConfig.useGpu)
            RegeneratePlanetGpu();
        else
            RegeneratePlanetCpu();
    }

    _guiManager.EndFrame();
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

    std::cout << "[Planet] CPU generated " << _planetMesh.GetVertexCount()
              << " vertices in " << _terrainStats.cpuTimeMs << " ms" << std::endl;
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
        auto shadingData = _terrainGenerator.GenerateShadingData(
            vertices, _genConfig.seed, _shadingSettings);
        meshData = render::MeshGenerator::GeneratePlanetMesh(
            _genConfig.subdivisions, _lodConfig.planetRadius, heights, shadingData);
    }
    else
    {
        meshData = render::MeshGenerator::GeneratePlanetMesh(
            _genConfig.subdivisions, _lodConfig.planetRadius, heights);
    }

    _planetMesh.Upload(meshData);

    auto end = std::chrono::high_resolution_clock::now();
    _terrainStats.gpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    std::cout << "[Planet] GPU generated " << _planetMesh.GetVertexCount()
              << " vertices in " << _terrainStats.gpuTimeMs << " ms" << std::endl;
}

void Application::RegenerateLodSystem()
{
    if (!_terrainGenerator.IsReady())
    {
        std::cerr << "[LOD] GPU compute shader required for LOD system" << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    _lodSystem.Initialize(_lodConfig.planetRadius, _lodConfig.patchSubdivisions,
                          _terrainGenerator, _terrainSettings, _shadingSettings,
                          _genConfig.seed);
    _oceanRenderer.Initialize(_lodConfig.planetRadius, _seaLevel, 5);
    _atmosphereRenderer.Initialize();

    auto end = std::chrono::high_resolution_clock::now();
    _terrainStats.gpuTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

    std::cout << "[LOD] Generated " << _lodSystem.GetPatchCount()
              << " patches in " << _terrainStats.gpuTimeMs << " ms" << std::endl;
}

} // namespace planets::app
