#include "Application.h"
#include "GpuConstants.h"
#include "gui/Widgets.h"
#include "Keybinds.h"
#include "planetgen/model/BodyConfigJson.h"
#include <nlohmann/json.hpp>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <random>

namespace planets::app
{

namespace
{
constexpr float HoldHintDelaySeconds = 0.2f; // short taps do not flash the hint
constexpr const char* BookmarksPath = "bookmarks.json";
}

Application::Application()
    : _seaLevel(AppDefaults::SeaLevel)
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
    if (!_capture.enabled)
        _help.LoadFirstLaunchState();

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
    _defaultScene = _sceneSettings;

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
        _cameraCtl.ResetView(CameraBodyNow(), _camera, /*eased=*/false);
        _bookmarks.Load(BookmarksPath);
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

namespace
{
// With keyboard nav on, WantCaptureKeyboard is true whenever any ImGui window has focus (the
// Planet window gets it at launch), which would kill every shortcut. Block only while a text
// field or another widget is being edited.
bool KeyboardBlockedByGui()
{
    return ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive();
}
} // namespace

void Application::HandleEscape(float deltaTime, bool keysBlocked)
{
    if (keysBlocked || !_input.IsKeyDown(Key::Escape))
    {
        _escHeld = 0.0f;
        _escConsumed = false;
        return;
    }

    // A press does one job: close help, else stop the turntable. It never quits by itself.
    if (Keybinds::Pressed(_input, Action::Escape))
    {
        if (_help.IsOpen())
        {
            _help.Close();
            _escConsumed = true;
        }
        else if (_cinematicController.IsPlaying())
        {
            StopCinematic();
            _escConsumed = true;
        }
    }
    if (_escConsumed)
        return;

    _escHeld += deltaTime;
    if (_escHeld >= Keybinds::QuitHoldSeconds)
        _window.SetShouldClose(true);
}

void Application::ProcessInput(float deltaTime)
{
    const bool keysBlocked = KeyboardBlockedByGui();

    HandleEscape(deltaTime, keysBlocked);

    if (!keysBlocked)
    {
        using Keybinds::Pressed;
        if (Pressed(_input, Action::Help))
            _help.Toggle();
        if (Pressed(_input, Action::HidePanel))
            _guiVisible = !_guiVisible;
        if (Pressed(_input, Action::Turntable))
        {
            if (_cinematicController.IsPlaying())
                StopCinematic();
            else
                StartCinematic();
        }
        if (Pressed(_input, Action::Screenshot))
            _screenshotRequested = true;
        if (Pressed(_input, Action::ToggleMode))
            _cameraCtl.ToggleMode(CameraBodyNow(), _camera);
        if (Pressed(_input, Action::AutoRotate))
        {
            core::CameraHud& hud = _camera.Hud();
            hud.autoRotate = !hud.autoRotate;
            // Auto-rotate turns the orbit, so leave Fly for it.
            if (hud.autoRotate && _camera.GetMode() != core::CameraMode::Orbit)
                _cameraCtl.ToggleMode(CameraBodyNow(), _camera);
        }
        if (Pressed(_input, Action::FramePlanet))
            _cameraCtl.FramePlanet(CameraBodyNow(), _camera);
        if (Pressed(_input, Action::ResetView))
            _cameraCtl.ResetView(CameraBodyNow(), _camera, /*eased=*/true);
        if (const int slot = Keybinds::DigitPressed(_input, Action::SaveBookmark))
            SaveBookmark(slot);
        if (const int slot = Keybinds::DigitPressed(_input, Action::RecallBookmark))
            RecallBookmark(slot);
        if (Pressed(_input, Action::Atmosphere))
            _atmosphereSettings.enabled = !_atmosphereSettings.enabled;
        if (Pressed(_input, Action::Reroll))
            ShuffleTerrain();
    }

    if (_cinematicController.IsPlaying())
    {
        _cinematicController.Update(deltaTime, _camera);
        if (!_cinematicController.IsPlaying())
            StopCinematic();
        return;
    }

    UpdateCamera(deltaTime, keysBlocked);
}

core::CameraBody Application::CameraBodyNow() const
{
    const float radius = PlanetRadius();
    core::CameraBody body;
    body.radius = radius;
    body.seaRadius = radius * _seaLevel;
    // Patch heights live on the GPU, so there is no cheap height under the camera. The floor is
    // the highest terrain the shape allows.
    const float heightScale = _activeBody ? _activeBody->GetHeightScale() : 0.0f;
    body.floorRadius = body.seaRadius + radius * heightScale * AppDefaults::HeightRangeMultiplier;
    body.startDistance = radius * AppDefaults::InitialCameraDistanceMultiplier;
    return body;
}

void Application::UpdateCamera(float deltaTime, bool keysBlocked)
{
    const ImGuiIO& io = ImGui::GetIO();
    core::CameraControl control;

    // A drag belongs to whoever got the press: a drag that starts on a panel never moves the camera.
    const bool left = _input.IsMouseButtonDown(MouseButton::Left);
    const bool right = _input.IsMouseButtonDown(MouseButton::Right);
    if (left && !_leftWasDown)
        _orbitDragging = !io.WantCaptureMouse;
    if (right && !_rightWasDown)
        _lookDragging = !io.WantCaptureMouse;
    _orbitDragging = _orbitDragging && left;
    _lookDragging = _lookDragging && right;
    _leftWasDown = left;
    _rightWasDown = right;

    const glm::vec2 mouseDelta(_input.GetMouseDeltaX(), _input.GetMouseDeltaY());
    if (_orbitDragging)
        control.orbitDrag = mouseDelta;
    if (_lookDragging)
        control.look = mouseDelta; // in Orbit this switches to Fly

    // Hide the cursor while looking, so the drag is not bounded by the window edge.
    const bool wantHidden = _lookDragging;
    if (wantHidden == _input.IsCursorEnabled())
        _input.SetCursorEnabled(!wantHidden);

    if (!io.WantCaptureMouse)
        control.zoomNotches = _input.GetScrollDelta();

    if (!keysBlocked)
    {
        using namespace Keybinds;
        const auto axis = [this](Key plus, Key minus) {
            return (_input.IsKeyDown(plus) ? 1.0f : 0.0f) - (_input.IsKeyDown(minus) ? 1.0f : 0.0f);
        };
        control.move = glm::vec3(axis(MoveRight, MoveLeft), axis(MoveUp, MoveDown), axis(MoveForward, MoveBack));
        if (_input.IsKeyDown(Faster))
            control.boost *= FasterFactor;
        if (_input.IsKeyDown(Slower))
            control.boost *= SlowerFactor;
    }
    control.autoRotateDegPerSec = _cinematicSettings.turntable.orbitSpeed;

    if (_input.WasDoubleClicked() && !io.WantCaptureMouse)
        FlyToCursor();

    _cameraCtl.Update(deltaTime, CameraBodyNow(), control, _camera);
}

void Application::FlyToCursor()
{
    int width = 0;
    int height = 0;
    glfwGetWindowSize(_window.GetHandle(), &width, &height);
    if (width <= 0 || height <= 0)
        return;

    const float ndcX = 2.0f * _input.GetMouseX() / static_cast<float>(width) - 1.0f;
    const float ndcY = 1.0f - 2.0f * _input.GetMouseY() / static_cast<float>(height);
    const glm::vec3 dir = core::CameraController::ScreenRay(_camera, ndcX, ndcY, _window.GetAspectRatio());

    // The sea-level sphere stands in for the terrain; the camera lands above the safe floor anyway.
    glm::vec3 hit;
    if (core::CameraController::RaySphere(_camera.GetPosition(), dir, PlanetRadius() * _seaLevel, hit))
        _cameraCtl.FlyToPoint(hit, CameraBodyNow(), _camera);
}

void Application::SaveBookmark(int slot)
{
    Bookmark bookmark;
    bookmark.used = true;
    bookmark.pose = _cameraCtl.CapturePose(_camera);
    bookmark.sunDir = _sceneSettings.lightDir;
    _bookmarks.Set(slot, bookmark);
    ShowToast(_bookmarks.Save(BookmarksPath) ? "Saved bookmark " + std::to_string(slot)
                                             : "Bookmark " + std::to_string(slot) + " not saved (file error)");
}

void Application::RecallBookmark(int slot)
{
    const Bookmark& bookmark = _bookmarks.Get(slot);
    if (!bookmark.used)
    {
        ShowToast("Bookmark " + std::to_string(slot) + " is empty");
        return;
    }
    _sceneSettings.lightDir = bookmark.sunDir;
    _cameraCtl.GoTo(bookmark.pose, _camera);
    ShowToast("Bookmark " + std::to_string(slot));
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
        const bool saved = _captureManager.CaptureScreenshot(capW, capH);
        _screenshotRequested = false;
        if (saved)
            ShowToast("Saved " + std::filesystem::absolute(_captureManager.GetLastCapturePath()).string());
        else
            ShowToast("Screenshot failed");
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

    if (!_cinematicController.IsPlaying())
    {
        _help.Draw();
        _toasts.Draw();
        if (_escHeld > HoldHintDelaySeconds)
            Toasts::DrawHoldHint("Hold Esc to quit", _escHeld / Keybinds::QuitHoldSeconds);
    }

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
        ui.moveSpeed = &_cameraCtl.SpeedMultiplier();
        ui.cinematic = &_cinematicSettings;
        ui.frameMs = _frameMs;
        ui.baseline = &_baseline;
        ui.lockMask = &_lockMask;
        ui.varietyUnits = _varietyUnits;

        const render::PlanetActions actions = _planetWindow.Draw(ui);
        bool needsRegen = actions.regenerate;
        if (actions.randomizeUnit >= 0)
            RandomizeUnit(static_cast<render::types::LookUnit>(actions.randomizeUnit));
        if (actions.resetUnit >= 0)
            ResetUnit(static_cast<render::types::LookUnit>(actions.resetUnit));
        // A seed typed into the panel is a new world for a planet type: roll its traits too.
        if (needsRegen && _genConfig.seed != _rolledSeed && RerollWorld())
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

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::MenuItem("Controls", "F1", _help.OpenFlag());
        ImGui::EndMenu();
    }
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
                SwitchBodyById(_bodies[i].id, /*keepLiveScene=*/false);
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
        if (!RerollWorld())
            RegenerateLodSystem();
    }
    ImGui::SetItemTooltip("Same seed + same type = same planet. Enter to apply.");
    // Amber '*': a lock, an edit or a tab Randomize means the seed alone no longer gives this planet.
    // Compared against the baseline, so a change made while its tab is closed counts too.
    if (_lockMask != 0 || _offSeedMask != 0 || render::types::ChangedUnits(CaptureLook(), _baseline) != 0)
    {
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextColored(render::ui::EditedColor, "*");
        ImGui::SetItemTooltip("Locked, edited or randomized tabs: the seed alone no longer gives this planet.");
    }

    if (ImGui::Button("Reroll"))
        ShuffleTerrain();
    ImGui::SetItemTooltip("New random seed. Unlocked tabs take new values. Key: R.");

    // Frame time on the right; the GPU split on hover. A dim hint sits to its left.
    char frame[48];
    std::snprintf(frame, sizeof(frame), "%.1f ms  %.0f fps", _frameMs, _frameMs > 0.0f ? 1000.0f / _frameMs : 0.0f);
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float frameX = ImGui::GetWindowWidth() - ImGui::CalcTextSize(frame).x - spacing * 2.0f;
    char hint[96];
    std::snprintf(hint, sizeof(hint), "%s  alt %.2f  speed %.2f   F1 help",
                  _camera.GetMode() == core::CameraMode::Orbit ? "Orbit" : "Fly", _camera.Hud().altitude,
                  _camera.Hud().speed);
    const float hintX = frameX - ImGui::CalcTextSize(hint).x - spacing * 4.0f;
    if (hintX > ImGui::GetCursorPosX())
    {
        ImGui::SameLine(hintX);
        ImGui::TextDisabled("%s", hint);
    }
    ImGui::SameLine(frameX);
    ImGui::TextUnformatted(frame);
    if (ImGui::IsItemHovered())
    {
        const render::GpuTimings gpu = _renderer.Timings();
        ImGui::SetTooltip("GPU terrain %.2f / ocean %.2f / effects %.2f ms", gpu.terrainMs, gpu.oceanMs,
                          gpu.effectsMs);
    }

    ImGui::EndMainMenuBar();
}

render::types::LookState Application::CaptureLook() const
{
    render::types::LookState s;
    if (_activeBody)
        s.config = _activeBody->Config();
    s.config.metadata.seaLevel = _seaLevel;
    s.atmosphere = _atmosphereSettings;
    s.clouds = _cloudSettings;
    s.ocean = _oceanSettings;
    s.scene = _sceneSettings;
    s.surface = _surfaceSettings;
    s.palette = _activePalette;
    return s;
}

void Application::ApplyLook(const render::types::LookState& s, bool rebuildBody, bool resetCamera)
{
    _atmosphereSettings = s.atmosphere;
    _cloudSettings = s.clouds;
    _oceanSettings = s.ocean;
    _sceneSettings = s.scene;
    _surfaceSettings = s.surface;
    _seaLevel = s.config.metadata.seaLevel;
    if (_capture.enabled)
        ApplyCaptureExposure();
    if (!rebuildBody)
        return;

    _activeBody = std::make_unique<render::BodyRuntime>(s.config, _paletteRegistry);
    _activePalette = s.palette;
    if (!_activePalette.empty())
        _activeBody->OverridePalette(_activePalette);

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

    RegenerateLodSystem();

    if (resetCamera)
    {
        float camDist = PlanetRadius() * AppDefaults::InitialCameraDistanceMultiplier;
        _camera.SetPosition(glm::vec3(0.0f, 0.0f, camDist));
        _cameraCtl.Cancel(_camera); // the controller re-adopts the pose next frame
        float farPlane = (std::max)(AppDefaults::MinFarPlane, PlanetRadius() * AppDefaults::FarPlaneRadiusMultiplier);
        _camera.SetFarPlane(farPlane);
    }
}

bool Application::RollWorld(uint32_t seed, uint32_t units, bool resetCamera, bool keepLiveScene)
{
    using namespace render::types;
    std::ifstream in(_activeBodyPath);
    const nlohmann::json body = nlohmann::json::parse(in, nullptr, false);
    if (body.is_discarded())
    {
        std::cerr << "[SwitchBody] Cannot read " << _activeBodyPath << std::endl;
        return false;
    }
    LookState rolled;
    const std::string err = RollLook(body, seed, keepLiveScene ? _sceneSettings : _defaultScene, rolled);
    if (!err.empty())
    {
        std::cerr << "[SwitchBody] Bad body config in " << _activeBodyPath << ": " << err << std::endl;
        return false;
    }
    _varietyUnits = VarietyUnits(body);

    // Units that do not take the roll keep their values and the default they are measured against.
    const uint32_t kept = AllUnits & ~units;
    LookState next = rolled;
    LookState baseline = rolled;
    if (_activeBody)
    {
        MergedRoll merged = MergeRoll(rolled, CaptureLook(), _baseline, kept);
        next = std::move(merged.next);
        baseline = std::move(merged.baseline);
    }
    _baseline = std::move(baseline);
    _planetWindow.ForgetEdits(units);

    const uint32_t bodyUnits = UnitBit(LookUnit::Terrain) | UnitBit(LookUnit::Surface) | UnitBit(LookUnit::Ocean);
    const bool rebuild = !_activeBody || (units & bodyUnits) != 0;
    ApplyLook(next, rebuild, resetCamera);
    if (rebuild)
        std::cout << "[SwitchBody] Switched to " << _activeBody->GetTypeName() << " (radius=" << PlanetRadius()
                  << ")" << std::endl;
    return true;
}

bool Application::SwitchBodyById(const std::string& id, bool keepLiveScene)
{
    const int index = render::types::FindBody(_bodies, id);
    if (index < 0)
    {
        std::cerr << "[SwitchBody] Unknown planet type '" << id << "'" << std::endl;
        return false;
    }
    // The combo and id change only once the switch worked.
    if (!SwitchBodyByPath(_bodies[index].path, /*resetCamera=*/true, keepLiveScene))
        return false;
    _selectedBodyType = index;
    _activeBodyId = id;
    return true;
}

bool Application::SwitchBodyByPath(const std::string& path, bool resetCamera, bool keepLiveScene)
{
    using namespace render::types;
    const std::string previous = _activeBodyPath;
    _activeBodyPath = path;
    // A new type keeps only a locked Light tab: the star and exposure are the user's, the rest is the type's.
    const uint32_t lockMask = _lockMask & UnitBit(LookUnit::Light);
    if (!RollWorld(_genConfig.seed, AllUnits & ~lockMask, resetCamera, keepLiveScene))
    {
        _activeBodyPath = previous;
        return false;
    }
    _lockMask = lockMask;
    _offSeedMask &= UnitBit(LookUnit::Light);
    _rolledSeed = _genConfig.seed;
    return true;
}

bool Application::RerollActiveBody()
{
    if (_varietyUnits == 0)
        return false;
    return RerollWorld(/*keepLiveScene=*/true);
}

bool Application::RerollWorld(bool keepLiveScene)
{
    using namespace render::types;
    const uint32_t units = AllUnits & ~_lockMask;
    if (!RollWorld(_genConfig.seed, units, /*resetCamera=*/false, keepLiveScene))
        return false;
    // Terrain, Surface and Ocean all locked: the body was kept, but the land still follows the seed.
    if ((units & (UnitBit(LookUnit::Terrain) | UnitBit(LookUnit::Surface) | UnitBit(LookUnit::Ocean))) == 0)
        RegenerateLodSystem();
    _offSeedMask &= ~units;
    _rolledSeed = _genConfig.seed;
    return true;
}

void Application::ShuffleTerrain()
{
    // A new world is a new seed: unlocked tabs take the type's values at that seed; Earth gets
    // new continents.
    _genConfig.seed = std::random_device{}();
    ShowToast("New seed " + std::to_string(_genConfig.seed));
    if (!RerollWorld())
        RegenerateLodSystem();
}

void Application::RandomizeUnit(render::types::LookUnit unit)
{
    // A fresh roll for one tab; the seed stays, so the planet no longer matches it alone.
    if (RollWorld(std::random_device{}(), render::types::UnitBit(unit), /*resetCamera=*/false, /*keepLiveScene=*/false))
        _offSeedMask |= render::types::UnitBit(unit);
}

void Application::ResetUnit(render::types::LookUnit unit)
{
    if (RollWorld(_genConfig.seed, render::types::UnitBit(unit), /*resetCamera=*/false, /*keepLiveScene=*/false))
        _offSeedMask &= ~render::types::UnitBit(unit);
}

void Application::StartCinematic()
{
    _cameraCtl.Cancel(_camera);
    _camera.Hud().autoRotate = false;
    _camera.SetOrbitTarget(glm::vec3(0.0f));
    _camera.SetMode(core::CameraMode::Orbit);
    _cinematicController.Start(_cinematicSettings, PlanetRadius(), _camera);
    _guiVisible = false;
}

void Application::StopCinematic()
{
    _cinematicController.Stop();
    _guiVisible = true;
    // The turntable orbits, so the camera stays in Orbit; the controller re-adopts the pose.
    _camera.SetMode(core::CameraMode::Orbit);
    _cameraCtl.Cancel(_camera);
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
