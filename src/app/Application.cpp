#include "Application.h"
#include "GpuConstants.h"
#include "instrument/ScriptRunner.h"
#include "instrument/TimelineParser.h"
#include "instrument/LiveServer.h"
#include <model/BodyConfigJson.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
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
                                                       (_lodConfig.planetRadius * _capture.distanceMultiplier);
        _camera.SetMode(core::CameraMode::FreeFly);
        _camera.SetPosition(eye);
        _camera.LookAt(target);
    }
    else
    {
        _camera.SetPosition(
            glm::vec3(0.0f, 0.0f, _lodConfig.planetRadius * AppDefaults::InitialCameraDistanceMultiplier));
    }

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

        SyncFramebufferToWindow();

        Render();
        _window.SwapBuffers();
    }
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

void Application::ApplyCaptureOverrides(const CaptureRequest& request)
{
    _capture = request;
}

void Application::ApplyCaptureExposure()
{
    // Brighten capture-mode exposure to a hero look (interactive defaults untouched).
    // Re-applied after a showcase regenerate, since RandomizeBodyParameters resets lighting.
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

bool Application::RunCapture(const CaptureRequest& request)
{
    ApplyCaptureOverrides(request);

    if (!Initialize())
    {
        std::cerr << "[Capture] Initialization failed." << std::endl;
        return false;
    }

    // Wait for the async LOD to fully populate before the shot. --frames is the settle floor;
    // draining guarantees no half-generated patch is captured.
    if (!DrainGeneration(_capture.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Capture] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; capturing anyway." << std::endl;

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

bool Application::RunCinematicCapture(const CaptureRequest& request)
{
    ApplyCaptureOverrides(request);

    if (!Initialize())
    {
        std::cerr << "[Cinematic] Initialization failed." << std::endl;
        return false;
    }

    // Drain the LOD so frame 1 is fully populated, not half-generated.
    if (!DrainGeneration(_capture.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Cinematic] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    // Configure the turntable from the request, then start it. Loop mode keeps the
    // controller from auto-stopping mid-sequence; the frame count defines the length.
    auto& tt = _cinematicSettings.turntable;
    if (_capture.orbitSpeed > 0.0f)
        tt.orbitSpeed = _capture.orbitSpeed;
    tt.startYaw = _capture.startYaw;
    tt.startPitch = _capture.startPitch;
    // --distance sets the orbit radius (multiplier of planet radius); hold it across the orbit.
    tt.zoomStartMultiplier = _capture.distanceMultiplier;
    tt.zoomEndMultiplier = _capture.distanceMultiplier;
    tt.durationMode = render::CinematicDurationMode::Loop;
    StartCinematic();

    // Fixed dt → deterministic, smooth motion independent of wall-clock.
    const float dt = 1.0f / (std::max)(1.0f, _capture.cinematicFps);
    const int frameCount = (std::max)(1, _capture.cinematicFrames);

    char path[512];
    int written = 0;
    for (int f = 0; f < frameCount; ++f)
    {
        _window.PollEvents();
        _cinematicController.Update(dt, _camera);
        _lastTime += dt; // keep shader time deterministic too (ocean waves etc.)

        // Keep the lit hemisphere facing the camera so the turntable never goes dark.
        if (_capture.lightFollow)
            UpdateFollowLight();

        std::snprintf(path, sizeof(path), _capture.framePattern.c_str(), f + 1); // 1-based
        _capture.outputPath = path;
        _headlessCaptureRequested = true;
        Render();
        _window.SwapBuffers();

        if (!_headlessCaptureRequested)
            ++written;
    }

    StopCinematic();

    std::cout << "[Cinematic] Wrote " << written << "/" << frameCount << " frames." << std::endl;
    Shutdown();
    return written == frameCount;
}

bool Application::RunGenerationShowcase(const CaptureRequest& request)
{
    ApplyCaptureOverrides(request);

    if (!Initialize())
    {
        std::cerr << "[Showcase] Initialization failed." << std::endl;
        return false;
    }

    if (!DrainGeneration(_capture.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Showcase] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    auto& tt = _cinematicSettings.turntable;
    if (_capture.orbitSpeed > 0.0f)
        tt.orbitSpeed = _capture.orbitSpeed;
    tt.zoomStartMultiplier = _capture.distanceMultiplier;
    tt.zoomEndMultiplier = _capture.distanceMultiplier;
    tt.durationMode = render::CinematicDurationMode::Loop;

    const float dt = 1.0f / (std::max)(1.0f, _capture.cinematicFps);
    const int planets = (std::max)(1, _capture.planets);
    const int hold = (std::max)(1, _capture.holdFrames);

    // Cycle through every planet type for visual contrast.
    const int typeCount = (std::max)(1, static_cast<int>(_bodies.size()));

    char path[512];
    int written = 0;
    int frameIndex = 0;
    for (int p = 0; p < planets; ++p)
    {
        // Each segment: switch to the next body type. Earth segments get randomized for
        // variety; volcanic/crystalline keep their authored identity. SwitchBody resets
        // the camera + radius, so re-seat the orbit afterwards.
        const int type = p % typeCount;
        // Earth segments use the authored earth.json with a stepped seed (reliably
        // land-facing, like the hero still). Volcanic/crystalline keep their identity.
        // Random earths placed their land on the back/poles too often to use here.
        if (type == 0)
            _genConfig.seed = _capture.seed + static_cast<uint32_t>(p);
        SwitchBodyById(_bodies.empty() ? "earth" : _bodies[type].id);

        ApplyCaptureExposure();
        StartCinematic(); // re-seat orbit target/distance for the new radius
        DrainGeneration(1, AppDefaults::CaptureDrainMaxFrames);

        for (int h = 0; h < hold; ++h, ++frameIndex)
        {
            _window.PollEvents();
            _cinematicController.Update(dt, _camera);
            _lastTime += dt;

            if (_capture.lightFollow)
                UpdateFollowLight();

            std::snprintf(path, sizeof(path), _capture.framePattern.c_str(), frameIndex + 1);
            _capture.outputPath = path;
            _headlessCaptureRequested = true;
            Render();
            _window.SwapBuffers();

            if (!_headlessCaptureRequested)
                ++written;
        }
    }

    StopCinematic();

    const int total = planets * hold;
    std::cout << "[Showcase] Wrote " << written << "/" << total << " frames across " << planets << " planets."
              << std::endl;
    Shutdown();
    return written == total;
}

planetgen::BodyConfig Application::LoadBodyConfigPublic(const std::string& path)
{
    return LoadBodyConfig(path);
}

bool Application::RunScript(const CaptureRequest& request)
{
    ApplyCaptureOverrides(request);

    std::ifstream in(_capture.scriptPath);
    if (!in)
    {
        std::cerr << "[Script] could not open '" << _capture.scriptPath << "'" << std::endl;
        return false;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::string err;
    auto timeline = instrument::ParseTimeline(text, err);
    if (!timeline)
    {
        std::cerr << "[Script] parse failed: " << err << std::endl;
        return false;
    }

    // setup block overrides the CLI/defaults before the window is built.
    if (timeline->hasSeed)
        _capture.seed = timeline->seed;
    if (timeline->hasBody)
        _capture.body = timeline->body;
    if (timeline->width > 0)
        _capture.width = timeline->width;
    if (timeline->height > 0)
        _capture.height = timeline->height;

    if (!Initialize())
    {
        std::cerr << "[Script] initialization failed." << std::endl;
        return false;
    }

    if (timeline->hasBody && _capture.body != _activeBodyId)
        SwitchBodyById(_capture.body);

    // Initial settle so the first command runs against a fully-generated planet.
    if (!DrainGeneration(_capture.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Script] generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    instrument::ScriptRunner runner(*this, timeline->fps);
    instrument::TimelineSource source(std::move(timeline->commands));
    runner.Run(source);

    std::cout << "[Script] timeline complete." << std::endl;
    Shutdown();
    return true;
}

bool Application::RunLive(const CaptureRequest& request)
{
    // Visible interactive window: keep capture-mode framing off so the user sees a normal
    // free-fly view; capture-on-demand still works via the _headlessCaptureRequested flag.
    _capture = request;
    _capture.enabled = false;

    if (!Initialize())
    {
        std::cerr << "[Live] initialization failed." << std::endl;
        return false;
    }

    if (!DrainGeneration(request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Live] generation did not drain; starting anyway." << std::endl;

    instrument::LiveCommandSource source;
    instrument::LiveServer server(source);
    if (!server.Start(request.livePort))
    {
        std::cerr << "[Live] could not start command server on port " << request.livePort << std::endl;
        Shutdown();
        return false;
    }

    // The runner mutates Application state on this (the render) thread only. fps sets the
    // fixed dt used by path/wait/burst commands; the wall-clock loop itself is free-running.
    instrument::ScriptRunner runner(*this, request.cinematicFps);

    std::cout << "[Live] ready — send JSON commands (one per line) to drive the planet." << std::endl;

    while (!_window.ShouldClose() && !source.IsClosed())
    {
        _window.PollEvents();
        _input.Update();
        ProcessInput(static_cast<float>(glfwGetTime()) - _lastTime);
        _lastTime = static_cast<float>(glfwGetTime());

        // Sync the framebuffer to the window size BEFORE dispatching commands: a capture/
        // burst command renders + reads pixels inside Dispatch, so the scene FBO + viewport
        // must already match the current window or the readback is garbled/wrong-aspect.
        SyncFramebufferToWindow();

        // Apply every command queued since the last frame, replying to each.
        instrument::LiveItem item;
        while (source.NextItem(item))
        {
            const std::string resp = runner.Dispatch(item.cmd);
            if (item.done)
                item.done->set_value(resp);
        }

        Render();
        _window.SwapBuffers();
    }

    server.Stop();
    std::cout << "[Live] session ended." << std::endl;
    Shutdown();
    return true;
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
    ctx.planetRadius = _lodConfig.planetRadius;
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

    if (_guiVisible)
    {
        auto& visibility = _guiManager.GetVisibility();

        // Body type selector
        if (ImGui::Begin("Body Type", nullptr, ImGuiWindowFlags_NoCollapse))
        {
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
            ImGui::SetItemTooltip("Planet types are the JSON files in data/bodies");

            if (_activeBody)
                ImGui::Text("Active: %s", _activeBody->GetTypeName().c_str());
        }
        ImGui::End();

        _scenePanel.Draw(_sceneSettings, _atmosphereSettings.enabled, visibility.scene);

        _terrainStats.patchCount = _renderer.QuadTree().GetActiveLeafCount();
        _terrainStats.visiblePatchCount = _renderer.QuadTree().GetVisiblePatchCount();
        const render::GpuTimings gpu = _renderer.Timings();
        _terrainStats.gpuTerrainMs = gpu.terrainMs;
        _terrainStats.gpuOceanMs = gpu.oceanMs;
        _terrainStats.gpuEffectsMs = gpu.effectsMs;

        bool randomize = false;
        bool needsRegen =
            _terrainPanel.Draw(_activeBody.get(), _genConfig, _lodConfig, _terrainStats, visibility.terrain, randomize);

        if (randomize && _activeBody)
        {
            render::RandomizeBodyParameters(
                _activeBody->Config(), _sceneSettings, _atmosphereSettings, _genConfig.seed);
            needsRegen = true;
        }

        needsRegen |= _surfacePanel.Draw(_activeBody.get(), _oceanSettings, _seaLevel, visibility.surface);
        _atmospherePanel.Draw(_atmosphereSettings, visibility.atmosphere);
        _cloudsPanel.Draw(_cloudSettings, _atmosphereSettings.enabled, visibility.clouds);
        _debugPanel.Draw(_camera, _moveSpeed, _autoOrbit, _autoOrbitSpeed, visibility.debug);

        bool playRequested = false;
        _cinematicPanel.Draw(_cinematicSettings, _lodConfig.planetRadius, playRequested, visibility.cinematic);
        if (playRequested)
            StartCinematic();

        if (needsRegen)
            RegenerateLodSystem();
    }

    _guiManager.EndFrame();
}

void Application::SwitchBody(planetgen::BodyConfig config, const render::types::BodyLook& look)
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

    _lodConfig.planetRadius = _activeBody->GetRadius();
    _seaLevel = _activeBody->GetSeaLevel();
    // Each planet type brings its own sky, clouds, ocean and star; a switch replaces them.
    render::types::ApplyBodyLook(look, _atmosphereSettings, _cloudSettings, _oceanSettings, _sceneSettings,
                                 _surfaceSettings);
    if (!look.palette.empty())
        _activeBody->OverridePalette(look.palette);
    _atmosphereSettings.enabled = _activeBody->HasAtmosphere() && look.atmosphere.enabled;
    if (_capture.enabled)
        ApplyCaptureExposure();

    RegenerateLodSystem();

    float camDist = _lodConfig.planetRadius * AppDefaults::InitialCameraDistanceMultiplier;
    _camera.SetPosition(glm::vec3(0.0f, 0.0f, camDist));
    float farPlane =
        (std::max)(AppDefaults::MinFarPlane, _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);

    std::cout << "[SwitchBody] Switched to " << _activeBody->GetTypeName() << " (radius=" << _lodConfig.planetRadius
              << ")" << std::endl;
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
    SwitchBody(LoadBodyConfig(_bodies[index].path), render::types::LoadBodyLook(_bodies[index].path));
    return true;
}

void Application::ShuffleTerrain()
{
    if (_activeBody)
        render::RandomizeBodyParameters(_activeBody->Config(), _sceneSettings, _atmosphereSettings, _genConfig.seed);
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

    float farPlane =
        (std::max)(AppDefaults::MinFarPlane, _lodConfig.planetRadius * AppDefaults::FarPlaneRadiusMultiplier);
    _camera.SetFarPlane(farPlane);
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
