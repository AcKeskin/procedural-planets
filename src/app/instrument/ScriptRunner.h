#pragma once

#include "Command.h"
#include "CameraPath.h"

namespace planets::app
{
class Application;
}

namespace planets::app::instrument
{

// A pull source of commands. The scripted front-end (TimelineSource) drains a parsed
// vector; a future live socket/MCP front-end is a second impl polling a thread-safe queue.
// The runner owns all GL/Application mutation; only the source differs.
class ICommandSource
{
public:
    virtual ~ICommandSource() = default;

    // Fetch the next command. Returns false when the source is exhausted (scripted) — a
    // live source blocks until a command is available or the session ends.
    virtual bool Next(Command& out) = 0;
};

// Drains the parsed timeline once.
class TimelineSource : public ICommandSource
{
public:
    explicit TimelineSource(std::vector<Command> commands) : _commands(std::move(commands)) {}
    bool Next(Command& out) override;

private:
    std::vector<Command> _commands;
    size_t _index = 0;
};

// Applies a command stream to an Application, on the render thread. Reuses the app's
// headless drain/regenerate/capture machinery. Friend of Application (see Application.h) so
// it mutates the private settings members without widening the public surface — the runner
// is an extension of the headless modes.
class ScriptRunner
{
public:
    ScriptRunner(Application& app, float fps);

    // Pull-and-dispatch until the source is exhausted.
    void Run(ICommandSource& source);

    // Dispatch a single command (used by Run and the live loop). Returns a short response
    // line — "ok", a written PNG path, or "error: ..." — for the live transport to reply.
    std::string Dispatch(const Command& cmd);

private:
    void ApplyCameraAbsolute(const CameraCommand& c);
    float CurrentRadius() const;        // active body radius (for distanceRadii); 10 if no body
    void ReapplyCamera();               // re-seat last camera intent (after a body switch resets it)
    void PumpFrames(int n);             // advance path + shader time, render, no capture
    void EnsureFresh();                 // regen + drain if dirty
    bool CaptureTo(const std::string& path);

    Application& _app;
    float _dt;
    bool _dirty = false;
    CameraPath _path;

    // Last camera the script set, re-applied after a setBody (SwitchBody resets the camera
    // to a body default, which would otherwise silently discard the user's framing).
    bool _hasCamera = false;
    CameraCommand _lastCamera;
};

} // namespace planets::app::instrument
