#include "Application.h"
#include "GpuConstants.h"
#include <model/BodyConfigJson.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <random>
#include <vector>

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
    {
        _genConfig.seed = _capture.seed;

        // Explicit --light aims the sun; otherwise the default light-axis framing applies.
        if (_capture.hasLight)
            _sceneSettings.lightDir = glm::normalize(_capture.lightDir);

        ApplyCaptureExposure();
    }

    // Load earth body from JSON and initialize
    auto earthCfg = LoadBodyConfig("data/bodies/earth.json");
    SwitchBody(std::move(earthCfg));

    // Initial generation (LOD-only; GPU is a hard requirement)
    RegenerateLodSystem();

    if (_capture.enabled)
    {
        // Default capture framing: sit on the light axis looking at the planet center, so the
        // fully-lit hemisphere faces the camera. An explicit --camera overrides the position but
        // still looks at origin (LookAt fixes the old -Z-only capture camera).
        const glm::vec3 target(0.0f);
        const glm::vec3 eye =
            _capture.hasCamera
                ? _capture.cameraPos
                : glm::normalize(_sceneSettings.lightDir) * (_lodConfig.planetRadius * _capture.distanceMultiplier);
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

void Application::ApplyCaptureExposure()
{
    // Brighten capture-mode exposure to a hero look (interactive defaults untouched).
    // Re-applied after a showcase regenerate, since RandomizeBodyParameters resets lighting.
    _sceneSettings.lighting.sunIntensity = AppDefaults::CaptureSunIntensity;
    _sceneSettings.lighting.ambientLight = AppDefaults::CaptureAmbientLight;
}

float Application::MeasureFrontLandFraction()
{
    if (!_libBody.IsValid())
        return 0.0f;

    // The "front" is the +lightDir hemisphere — the lit face the camera looks at. Sample an
    // even sphere, generate heights, but only score points on that hemisphere so a planet
    // with land hidden on the back/poles doesn't pass.
    const glm::vec3 front = glm::normalize(_sceneSettings.lightDir);
    constexpr int kSamples = 512;
    constexpr float kGolden = 2.39996323f;
    std::vector<float> verts;
    std::vector<glm::vec3> dirs;
    verts.reserve(kSamples * 3);
    dirs.reserve(kSamples);
    for (int i = 0; i < kSamples; ++i)
    {
        const float y = 1.0f - 2.0f * (i + 0.5f) / kSamples;
        const float r = std::sqrt((std::max)(0.0f, 1.0f - y * y));
        const float theta = kGolden * i;
        const glm::vec3 d(r * std::cos(theta), y, r * std::sin(theta));
        dirs.push_back(d);
        verts.push_back(d.x);
        verts.push_back(d.y);
        verts.push_back(d.z);
    }

    pg::Result res = _libBody.GenerateHeights(verts.data(), kSamples, _genConfig.seed);
    const float* heights = res.IsValid() ? res.Heights() : nullptr;
    if (!heights)
        return 0.0f;

    int frontPoints = 0, frontLand = 0;
    const uint32_t n = res.Count();
    for (uint32_t i = 0; i < n && i < dirs.size(); ++i)
    {
        if (glm::dot(dirs[i], front) <= 0.25f) // only the camera-facing cap
            continue;
        ++frontPoints;
        if (heights[i] > _seaLevel)
            ++frontLand;
    }
    return frontPoints > 0 ? static_cast<float>(frontLand) / static_cast<float>(frontPoints) : 0.0f;
}

void Application::UpdateFollowLight()
{
    // Match the well-lit still's geometry: the lit hemisphere faces the camera when the
    // light direction points from the planet toward the eye (eye ∥ +lightDir). Tilt it
    // up-and-to-the-side for a key-light terminator (shape, not a flat disc).
    const glm::vec3 toEye = glm::normalize(_camera.GetPosition()); // camera looks at origin
    const glm::vec3 right = _camera.GetRight();
    const glm::vec3 up = _camera.GetUp();
    // Small offset: enough terminator for shape, but keep most of the disc (incl. limbs
    // where land sits during the orbit) lit.
    _sceneSettings.lightDir = glm::normalize(toEye + up * 0.18f + right * 0.12f);
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
        std::cerr << "[Cinematic] Generation did not drain within "
                  << AppDefaults::CaptureDrainMaxFrames << " frames; starting anyway." << std::endl;

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
        std::cerr << "[Showcase] Generation did not drain within "
                  << AppDefaults::CaptureDrainMaxFrames << " frames; starting anyway." << std::endl;

    auto& tt = _cinematicSettings.turntable;
    if (_capture.orbitSpeed > 0.0f)
        tt.orbitSpeed = _capture.orbitSpeed;
    tt.zoomStartMultiplier = _capture.distanceMultiplier;
    tt.zoomEndMultiplier = _capture.distanceMultiplier;
    tt.durationMode = render::CinematicDurationMode::Loop;

    const float dt = 1.0f / (std::max)(1.0f, _capture.cinematicFps);
    const int planets = (std::max)(1, _capture.planets);
    const int hold = (std::max)(1, _capture.holdFrames);

    // Cycle the three body types for visual contrast (blue earth / red volcanic /
    // crystalline), randomizing earth segments for extra variety.
    static const char* bodyPaths[] = {
        "data/bodies/earth.json", "data/bodies/volcanic.json", "data/bodies/crystalline.json"};

    char path[512];
    int written = 0;
    int frameIndex = 0;
    for (int p = 0; p < planets; ++p)
    {
        // Each segment: switch to the next body type. Earth segments get randomized for
        // variety; volcanic/crystalline keep their authored identity. SwitchBody resets
        // the camera + radius, so re-seat the orbit afterwards.
        const int type = p % 3;
        SwitchBody(LoadBodyConfig(bodyPaths[type]));
        if (type == 0 && p > 0)
        {
            // Re-roll the random earth until enough land faces the camera (not just total
            // land — that was hiding continents on the back/poles). Keeps shape random.
            ShuffleTerrain();
            for (int attempt = 1; attempt < AppDefaults::ShowcaseMaxLandRolls &&
                                  MeasureFrontLandFraction() < AppDefaults::ShowcaseMinFrontLand;
                 ++attempt)
                ShuffleTerrain();
        }

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
    std::cout << "[Showcase] Wrote " << written << "/" << total << " frames across "
              << planets << " planets." << std::endl;
    Shutdown();
    return written == total;
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
            _terrainPanel.Draw(_activeBody.get(), _genConfig, _lodConfig, _terrainStats, visibility.terrain, randomize);

        if (randomize && _activeBody)
        {
            render::RandomizeBodyParameters(_activeBody->Config(), _sceneSettings,
                                            _atmosphereSettings, _genConfig.seed);
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
    if (_activeBody)
        render::RandomizeBodyParameters(_activeBody->Config(), _sceneSettings,
                                        _atmosphereSettings, _genConfig.seed);
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
