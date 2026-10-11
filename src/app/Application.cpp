#include "Application.h"
#include "GpuConstants.h"
#include "planetgen/model/BodyConfigJson.h"
#include "planetgen/model/BodyVariety.h"
#include <nlohmann/json.hpp>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <cstdio>
#include <iostream>
#include <fstream>
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
        std::cerr << "[Application] FATAL: failed to create libplanetgen context: " << e.what() << std::endl;
        return false;
    }

    // Capture mode: seed must be set before generation so the shot is deterministic.
    if (_capture.enabled)
    {
        _genConfig.seed = _capture.seed;

        // Explicit --light aims the sun; otherwise the default light-axis framing applies.
        if (_capture.hasLight)
            _sceneSettings.lightDir = glm::normalize(_capture.lightDir);

        ApplyCaptureExposure();
    }

    // Planet types are the JSON files in data/bodies; start on the requested one (Earth by default).
    _bodies = render::types::ScanBodies("data/bodies");
    if (!SwitchBodyById(_capture.body) && !SwitchBodyById("earth") && !_bodies.empty())
        SwitchBodyById(_bodies.front().id);

    // Initial generation (LOD-only; GPU is a hard requirement)
    RegenerateLodSystem();

    if (_capture.enabled)
    {
        // Default capture framing: sit on the sun side of the light axis looking at the planet
        // center, so the lit hemisphere faces the camera (lightDir points FROM the sun). An explicit
        // --camera overrides the position but still looks at origin.
        const glm::vec3 target(0.0f);
        const glm::vec3 eye = _capture.hasCamera ? _capture.cameraPos
                                                 : -glm::normalize(_sceneSettings.lightDir) *
                                                       (PlanetRadius() * _capture.distanceMultiplier);
        _camera.SetMode(core::CameraMode::FreeFly);
        _camera.SetPosition(eye);
        _camera.LookAt(target);
    }
    else
    {
        _camera.SetPosition(glm::vec3(0.0f, 0.0f, PlanetRadius() * AppDefaults::InitialCameraDistanceMultiplier));
    }

    _lastTime = static_cast<float>(glfwGetTime());
    _previousWidth = _window.GetWidth();
    _previousHeight = _window.GetHeight();

    return true;
}

void Application::Run()
{
    while (!ShouldClose())
    {
        BeginInteractiveFrame();
        PresentFrame();
    }
}

bool Application::Boot(const CaptureRequest& request)
{
    _capture = request;
    return Initialize();
}

bool Application::ShouldClose() const
{
    return _window.ShouldClose();
}

void Application::BeginInteractiveFrame()
{
    const float now = static_cast<float>(glfwGetTime());
    const float deltaTime = now - _lastTime;
    _lastTime = now;
    _frameMs += (deltaTime * 1000.0f - _frameMs) * 0.1f;

    _window.PollEvents();
    _input.Update();
    ProcessInput(deltaTime);

    // Before anything renders this frame: a live capture reads pixels from the scene FBO,
    // which must already match the window size.
    SyncFramebufferToWindow();
}

void Application::PresentFrame()
{
    Render();
    _window.SwapBuffers();
}

bool Application::CaptureFrame(const std::string& path)
{
    if (!path.empty())
        _capture.outputPath = path;
    _headlessCaptureRequested = true; // Render() writes the PNG at the pre-GUI point and clears it
    PresentFrame();
    return !_headlessCaptureRequested;
}

void Application::StepCinematic(float dt)
{
    _window.PollEvents();
    _cinematicController.Update(dt, _camera);
    _lastTime += dt; // keep shader time deterministic too (ocean waves etc.)

    // Keep the lit hemisphere facing the camera so the turntable never goes dark.
    if (_capture.lightFollow)
        UpdateFollowLight();
}

void Application::SyncFramebufferToWindow()
{
    const int w = _window.GetWidth();
    const int h = _window.GetHeight();
    if (w == _previousWidth && h == _previousHeight)
        return;
    if (w <= 0 || h <= 0)
        return; // minimized; skip until restored
    _renderer.ResizeFramebuffer(w, h);
    _previousWidth = w;
    _previousHeight = h;
}

void Application::ApplyCaptureExposure()
{
    // Brighten capture-mode exposure to a hero look (interactive defaults untouched).
    // Re-applied after a body switch, since the body's look sets its own lighting.
    _sceneSettings.lighting.sunIntensity = AppDefaults::CaptureSunIntensity;
    _sceneSettings.lighting.ambientLight = AppDefaults::CaptureAmbientLight;
}

void Application::UpdateFollowLight()
{
    // Keep the lit hemisphere facing the camera: lightDir points FROM the sun, so it points from
    // the eye toward the planet (eye ∥ -lightDir). Tilt it up-and-to-the-side for a key-light
    // terminator (shape, not a flat disc).
    const glm::vec3 toEye = glm::normalize(_camera.GetPosition()); // camera looks at origin
    const glm::vec3 right = _camera.GetRight();
    const glm::vec3 up = _camera.GetUp();
    // Small offset: enough terminator for shape, but keep most of the disc (incl. limbs
    // where land sits during the orbit) lit.
    _sceneSettings.lightDir = -glm::normalize(toEye + up * 0.18f + right * 0.12f);
}

bool Application::DrainGeneration(int minFrames, int maxFrames)
{
    minFrames = (std::max)(1, minFrames);
    maxFrames = (std::max)(minFrames, maxFrames);

    // The quad-tree enqueues more split work as the camera-appropriate LOD resolves, so
    // pump frames until the scheduler reports nothing pending (a few clean frames in a row,
    // to absorb the one-frame gap between completion readback and the next Update enqueue).
    int cleanStreak = 0;
    int frame = 0;
    for (; frame < maxFrames; ++frame)
    {
        _window.PollEvents();
        Render();
        _window.SwapBuffers();

        const bool busy = _renderer.Scheduler().HasPendingWork();
        cleanStreak = busy ? 0 : cleanStreak + 1;

        // Settled: queue empty for a couple of frames AND past the floor.
        if (frame + 1 >= minFrames && cleanStreak >= 2)
            return true;
    }
    return false;
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
    render::RenderContext ctx;
    ctx.view = _camera.GetViewMatrix();
    ctx.projection = _camera.GetProjectionMatrix(_window.GetAspectRatio());
    ctx.cameraPos = _camera.GetPosition();
    ctx.nearPlane = _camera.GetNearPlane();
    ctx.farPlane = _camera.GetFarPlane();
    ctx.time = _lastTime;
    ctx.scene = &_sceneSettings;
    ctx.ocean = &_oceanSettings;
    ctx.surface = &_surfaceSettings;
    ctx.atmosphere = &_atmosphereSettings;
    ctx.clouds = &_cloudSettings;
    ctx.body = _activeBody.get();
    ctx.planetRadius = PlanetRadius();
    ctx.seaLevel = _seaLevel;

    _renderer.DrawScene(ctx);

    // Read pixels at the size the renderer actually drew + presented, not the window size
    // (which can differ if a resize landed between this draw and the readback in live mode).
    const int capW = _renderer.FramebufferWidth();
    const int capH = _renderer.FramebufferHeight();

    if (_screenshotRequested)
    {
        _captureManager.CaptureScreenshot(capW, capH);
        _screenshotRequested = false;
    }

    // Headless capture point: scene composited, before the GUI overlay.
    if (_headlessCaptureRequested)
    {
        _captureManager.CaptureScreenshotToFile(_capture.outputPath, capW, capH);
        _headlessCaptureRequested = false;
    }

    RenderGui();
}

void Application::RenderGui()
{
    _guiManager.BeginFrame();

    // The bar stays when Tab hides the Planet window; a playing turntable hides everything.
    if (!_cinematicController.IsPlaying())
        DrawBar();
    _guiManager.DrawDockspace();

    if (_guiVisible)
    {
        _terrainStats.patchCount = _renderer.QuadTree().GetActiveLeafCount();
        _terrainStats.visiblePatchCount = _renderer.QuadTree().GetVisiblePatchCount();
        const render::GpuTimings gpu = _renderer.Timings();
        _terrainStats.gpuTerrainMs = gpu.terrainMs;
        _terrainStats.gpuOceanMs = gpu.oceanMs;
        _terrainStats.gpuEffectsMs = gpu.effectsMs;

        render::PlanetUi ui;
        ui.body = _activeBody.get();
        ui.gen = &_genConfig;
        ui.lod = &_lodConfig;
        ui.stats = &_terrainStats;
        ui.surface = &_surfaceSettings;
        ui.ocean = &_oceanSettings;
        ui.seaLevel = &_seaLevel;
        ui.atmosphere = &_atmosphereSettings;
        ui.clouds = &_cloudSettings;
        ui.scene = &_sceneSettings;
        ui.camera = &_camera;
        ui.moveSpeed = &_moveSpeed;
        ui.autoOrbit = _autoOrbit;
        ui.autoOrbitSpeed = &_autoOrbitSpeed;
        ui.cinematic = &_cinematicSettings;
        ui.planetRadius = PlanetRadius();

        const render::PlanetActions actions = _planetWindow.Draw(ui);
        bool needsRegen = actions.regenerate;
        if (actions.randomize && _activeBody)
            ShuffleTerrain();
        // A seed typed into the panel is a new world for a planet type: roll its traits too.
        if (needsRegen && _activeHasVariety && _genConfig.seed != _rolledSeed && RerollActiveBody())
            needsRegen = false;
        if (actions.playCinematic)
            StartCinematic();
        if (needsRegen)
            RegenerateLodSystem();
    }

    _guiManager.EndFrame();
}

void Application::DrawBar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Planet window", "Tab", &_guiVisible);
        if (ImGui::MenuItem("Reset Layout"))
            _guiManager.RequestResetLayout();
        ImGui::EndMenu();
    }
    ImGui::Separator();

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0f);
    const char* preview = _bodies.empty() ? "" : _bodies[_selectedBodyType].name.c_str();
    if (ImGui::BeginCombo("Type", preview))
    {
        for (int i = 0; i < static_cast<int>(_bodies.size()); ++i)
        {
            if (ImGui::Selectable(_bodies[i].name.c_str(), i == _selectedBodyType) && i != _selectedBodyType)
                SwitchBodyById(_bodies[i].id);
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Planet type. Each type sets its own terrain, surface, sky and star.");

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0f);
    uint32_t seed = _genConfig.seed;
    if (ImGui::InputScalar("Seed", ImGuiDataType_U32, &seed, nullptr, nullptr, "%u",
                           ImGuiInputTextFlags_EnterReturnsTrue) &&
        seed != _genConfig.seed)
    {
        _genConfig.seed = seed;
        if (!RerollActiveBody())
            RegenerateLodSystem();
    }
    ImGui::SetItemTooltip("Same seed + same type = same planet. Enter to apply.");

    if (ImGui::Button("Reroll"))
        ShuffleTerrain();
    ImGui::SetItemTooltip("New random seed. Key: R.");

    // Frame time on the right; the GPU split on hover.
    char frame[48];
    std::snprintf(frame, sizeof(frame), "%.1f ms  %.0f fps", _frameMs, _frameMs > 0.0f ? 1000.0f / _frameMs : 0.0f);
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(frame).x - ImGui::GetStyle().ItemSpacing.x * 2.0f);
    ImGui::TextUnformatted(frame);
    if (ImGui::IsItemHovered())
    {
        const render::GpuTimings gpu = _renderer.Timings();
        ImGui::SetTooltip("GPU terrain %.2f / ocean %.2f / effects %.2f ms", gpu.terrainMs, gpu.oceanMs,
                          gpu.effectsMs);
    }

    ImGui::EndMainMenuBar();
}

void Application::SwitchBody(planetgen::BodyConfig config, const render::types::BodyLook& look, bool resetCamera)
{
    _activeBody = std::make_unique<render::BodyRuntime>(std::move(config), _paletteRegistry);

    // The library owns all terrain compute; hand it the full config as data so it
    // bakes the continent mask from the in-memory (GUI-edited) config.
    {
        const std::string configJson = planetgen::BodyConfigToJson(_activeBody->Config());
        _libBody = _libContext->CreateBodyFromJsonString(configJson);
        if (!_libBody.IsValid())
        {
            std::cerr << "[SwitchBody] FATAL: libplanetgen failed to create body for " << _activeBody->GetTypeName()
                      << ": " << _libContext->GetErrorMessage() << " — aborting." << std::endl;
            std::abort();
        }
    }
    _terrainStats.gpuAvailable = true;

    _seaLevel = _activeBody->GetSeaLevel();
    // Each planet type brings its own sky, clouds, ocean and star; a switch replaces them.
    render::types::ApplyBodyLook(
        look, _atmosphereSettings, _cloudSettings, _oceanSettings, _sceneSettings, _surfaceSettings);
    if (!look.palette.empty())
        _activeBody->OverridePalette(look.palette);
    _atmosphereSettings.enabled = _activeBody->HasAtmosphere() && look.atmosphere.enabled;
    if (_capture.enabled)
        ApplyCaptureExposure();

    RegenerateLodSystem();

    if (resetCamera)
    {
        float camDist = PlanetRadius() * AppDefaults::InitialCameraDistanceMultiplier;
        _camera.SetPosition(glm::vec3(0.0f, 0.0f, camDist));
        float farPlane = (std::max)(AppDefaults::MinFarPlane, PlanetRadius() * AppDefaults::FarPlaneRadiusMultiplier);
        _camera.SetFarPlane(farPlane);
    }

    std::cout << "[SwitchBody] Switched to " << _activeBody->GetTypeName() << " (radius=" << PlanetRadius() << ")"
              << std::endl;
}

bool Application::SwitchBodyById(const std::string& id)
{
    const int index = render::types::FindBody(_bodies, id);
    if (index < 0)
    {
        std::cerr << "[SwitchBody] Unknown planet type '" << id << "'" << std::endl;
        return false;
    }
    _selectedBodyType = index;
    _activeBodyId = id;
    return SwitchBodyByPath(_bodies[index].path);
}

bool Application::SwitchBodyByPath(const std::string& path, bool resetCamera)
{
    std::ifstream in(path);
    const nlohmann::json body = nlohmann::json::parse(in, nullptr, false);
    if (body.is_discarded())
    {
        std::cerr << "[SwitchBody] Cannot read " << path << std::endl;
        return false;
    }
    const nlohmann::json world = planetgen::RollVariety(body, _genConfig.seed);
    planetgen::BodyConfig config;
    const std::string err = planetgen::BodyConfigFromJson(world.dump(), config);
    if (!err.empty())
    {
        std::cerr << "[SwitchBody] Bad body config in " << path << ": " << err << std::endl;
        return false;
    }
    _activeBodyPath = path;
    _activeHasVariety = planetgen::HasVariety(body);
    _rolledSeed = _genConfig.seed;
    SwitchBody(std::move(config), render::types::ParseBodyLook(world), resetCamera);
    return true;
}

bool Application::RerollActiveBody()
{
    if (!_activeHasVariety)
        return false;
    return SwitchBodyByPath(_activeBodyPath, /*resetCamera=*/false);
}

void Application::ShuffleTerrain()
{
    // A new world is a new seed: a type with variety rolls its traits again; Earth keeps its
    // settings and gets new continents from the seed.
    _genConfig.seed = std::random_device{}();
    if (!RerollActiveBody())
        RegenerateLodSystem();
}

void Application::StartCinematic()
{
    _camera.SetOrbitTarget(glm::vec3(0.0f));
    _camera.SetMode(core::CameraMode::Orbit);
    _cinematicController.Start(_cinematicSettings, PlanetRadius(), _camera);
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

    // Recreate the library body so it re-bakes the continent mask from the current
    // (possibly GUI-edited) config.
    if (_libContext)
    {
        const std::string configJson = planetgen::BodyConfigToJson(_activeBody->Config());
        pg::Body rebuilt = _libContext->CreateBodyFromJsonString(configJson);
        if (rebuilt.IsValid())
            _libBody = std::move(rebuilt);
    }

    _renderer.SetActiveBody(_libBody.Handle(), *_activeBody, BuildQuadTreeConfig(), _genConfig.seed);

    float farPlane = (std::max)(AppDefaults::MinFarPlane, PlanetRadius() * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);
}

float Application::PlanetRadius() const
{
    return _activeBody ? _activeBody->GetRadius() : planetgen::BodyConfig{}.metadata.radius;
}

render::lod::QuadTreeConfig Application::BuildQuadTreeConfig() const
{
    render::lod::QuadTreeConfig config;
    config.planetRadius = PlanetRadius();
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
