#include "ScriptRunner.h"

#include "ParamRegistry.h"
#include "../Application.h"

#include "planetgen/model/BodyConfig.h"
#include "planetgen/model/BodyConfigJson.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace planets::app::instrument
{

bool TimelineSource::Next(Command& out)
{
    if (_index >= _commands.size())
        return false;
    out = _commands[_index++];
    return true;
}

ScriptRunner::ScriptRunner(Application& app, float fps)
    : _app(app)
    , _dt(1.0f / (std::max)(1.0f, fps))
{
}

void ScriptRunner::Run(ICommandSource& source)
{
    Command cmd;
    while (source.Next(cmd))
        Dispatch(cmd);
}

std::string ScriptRunner::Dispatch(const Command& cmd)
{
    switch (cmd.type)
    {
    case CommandType::SetSeed:
    {
        _app._genConfig.seed = std::get<SetSeedCommand>(cmd.payload).seed;
        // A planet type is type + seed: a new seed rolls its traits again.
        if (_app.RerollActiveBody())
        {
            _dirty = false;
            ReapplyCamera();
            return "ok (rerolled)";
        }
        _dirty = true;
        return "ok (dirty)";
    }
    case CommandType::SetBody:
    {
        const auto& c = std::get<SetBodyCommand>(cmd.payload);
        // By id through the registry; an explicit path loads that file directly.
        if (!c.path.empty())
        {
            if (!_app.SwitchBodyByPath(c.path))
                return "error: cannot load '" + c.path + "'";
        }
        else if (!_app.SwitchBodyById(c.body))
            return "error: unknown body '" + c.body + "'";
        _dirty = false;
        ReapplyCamera(); // SwitchBody reset the camera to a body default — restore our framing
        return "ok (body=" + (c.body.empty() ? c.path : c.body) + ")";
    }
    case CommandType::SetParam:
    {
        const auto& c = std::get<SetParamCommand>(cmd.payload);
        ParamTargets t;
        t.scene = &_app._sceneSettings;
        t.ocean = &_app._oceanSettings;
        t.atmosphere = &_app._atmosphereSettings;
        t.clouds = &_app._cloudSettings;
        t.surface = &_app._surfaceSettings;
        t.lod = &_app._lodConfig;
        t.gen = &_app._genConfig;
        t.seaLevel = &_app._seaLevel;
        t.body = _app._activeBody ? &_app._activeBody->Config() : nullptr;

        const ApplyResult r = ApplyParam(t, c.key, c.value);
        if (!r.applied)
        {
            std::cerr << "[Script] could not apply param '" << c.key << "'" << std::endl;
            return "error: could not apply '" + c.key + "'";
        }
        _dirty |= r.needsRegen;
        return r.needsRegen ? "ok (dirty: " + c.key + ")" : "ok (" + c.key + ")";
    }
    case CommandType::Camera:
    {
        const auto& c = std::get<CameraCommand>(cmd.payload);
        _lastCamera = c;
        _hasCamera = true;
        if (c.mode == CameraMode::Path)
            _path.Build(c, CurrentRadius());
        else
            ApplyCameraAbsolute(c);
        return "ok (camera)";
    }
    case CommandType::Wait:
    {
        PumpFrames(std::get<WaitCommand>(cmd.payload).frames);
        return "ok (waited)";
    }
    case CommandType::Capture:
    {
        const std::string& path = std::get<CaptureCommand>(cmd.payload).path;
        return CaptureTo(path) ? path : ("error: capture failed for " + path);
    }
    case CommandType::Burst:
    {
        const auto& c = std::get<BurstCommand>(cmd.payload);
        EnsureFresh();
        char path[512];
        int shot = 0;
        for (int f = 0; f < c.frames; ++f)
        {
            if (_path.IsArmed())
                _path.Advance(_dt, _app._camera);
            _app._lastTime += _dt;
            if (f % c.every == 0)
            {
                std::snprintf(path, sizeof(path), c.pattern.c_str(), shot++);
                _app._capture.outputPath = path;
                _app._headlessCaptureRequested = true;
            }
            _app.Render();
            _app._window.SwapBuffers();
            _app._window.PollEvents();
        }
        std::cout << "[Script] burst wrote " << shot << " frames (" << c.pattern << ")" << std::endl;
        return "ok (burst: " + std::to_string(shot) + " frames)";
    }
    }
    return "ok";
}

void ScriptRunner::ApplyCameraAbsolute(const CameraCommand& c)
{
    _path.Reset();
    auto& cam = _app._camera;
    switch (c.mode)
    {
    case CameraMode::LookAt:
        cam.SetMode(core::CameraMode::FreeFly);
        cam.SetPosition(c.eye);
        cam.LookAt(c.target);
        break;
    case CameraMode::FreeFly:
    {
        // Seat orientation by aiming at a point one unit along the yaw/pitch direction
        // (matches Camera::UpdateVectors' forward convention). LookAt back-solves the Euler.
        cam.SetMode(core::CameraMode::FreeFly);
        cam.SetPosition(c.eye);
        const float yawR = glm::radians(c.yaw);
        const float pitchR = glm::radians(c.pitch);
        const glm::vec3 dir{std::cos(yawR) * std::cos(pitchR), std::sin(pitchR), std::sin(yawR) * std::cos(pitchR)};
        cam.LookAt(c.eye + dir);
        break;
    }
    case CameraMode::Orbit:
    {
        cam.SetMode(core::CameraMode::Orbit);
        cam.SetOrbitTarget(c.target);
        const float dist = c.distanceRadii > 0.0f ? CurrentRadius() * c.distanceRadii : c.distance;
        if (dist > 0.0f)
            cam.SetOrbitDistance(dist);
        cam.SetOrbitAngles(c.yaw, c.pitch);
        break;
    }
    case CameraMode::Path:
        break; // handled by Build in Dispatch
    }
}

void ScriptRunner::ReapplyCamera()
{
    if (!_hasCamera)
        return;
    if (_lastCamera.mode == CameraMode::Path)
        _path.Build(_lastCamera, CurrentRadius());
    else
        ApplyCameraAbsolute(_lastCamera);
}

float ScriptRunner::CurrentRadius() const
{
    return _app.PlanetRadius();
}

void ScriptRunner::PumpFrames(int n)
{
    for (int f = 0; f < (std::max)(1, n); ++f)
    {
        if (_path.IsArmed())
            _path.Advance(_dt, _app._camera);
        _app._lastTime += _dt;
        _app.Render();
        _app._window.SwapBuffers();
        _app._window.PollEvents();
    }
}

void ScriptRunner::EnsureFresh()
{
    if (!_dirty)
        return;
    _app.RegenerateLodSystem();
    _app.DrainGeneration(1, AppDefaults::CaptureDrainMaxFrames);
    _dirty = false;
}

bool ScriptRunner::CaptureTo(const std::string& path)
{
    EnsureFresh();
    if (_path.IsArmed())
        _path.Advance(0.0f, _app._camera); // re-seat at current clock without advancing
    _app._capture.outputPath = path;
    _app._headlessCaptureRequested = true;
    _app.Render();
    _app._window.SwapBuffers();
    _app._window.PollEvents();
    const bool ran = !_app._headlessCaptureRequested;
    if (!ran)
        std::cerr << "[Script] capture did not run for '" << path << "'" << std::endl;

    // Logged per capture so scripted runs double as perf probes.
    const render::GpuTimings gpu = _app._renderer.Timings();
    std::cout << "[Script] gpu ms: terrain " << gpu.terrainMs << ", ocean " << gpu.oceanMs << ", effects "
              << gpu.effectsMs << ", total " << gpu.TotalMs() << " (" << path << ")" << std::endl;
    return ran;
}

} // namespace planets::app::instrument
