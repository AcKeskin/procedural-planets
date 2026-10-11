#include "RunModes.h"
#include "Application.h"
#include "instrument/LiveServer.h"
#include "instrument/ScriptRunner.h"

#include <iostream>

namespace planets::app
{

bool LiveMode::Run(Application& app)
{
    // Visible interactive window: keep capture-mode framing off so the user sees a normal
    // free-fly view; capture-on-demand still works through CaptureFrame.
    CaptureRequest request = _request;
    request.enabled = false;

    if (!app.Boot(request))
    {
        std::cerr << "[Live] initialization failed." << std::endl;
        return false;
    }

    if (!app.DrainGeneration(request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Live] generation did not drain; starting anyway." << std::endl;

    instrument::LiveCommandSource source;
    instrument::LiveServer server(source);
    if (!server.Start(request.livePort))
    {
        std::cerr << "[Live] could not start command server on port " << request.livePort << std::endl;
        app.Shutdown();
        return false;
    }

    // The runner mutates Application state on this (the render) thread only. fps sets the
    // fixed dt used by path/wait/burst commands; the wall-clock loop itself is free-running.
    instrument::ScriptRunner runner(app, request.cinematicFps);

    std::cout << "[Live] ready — send JSON commands (one per line) to drive the planet." << std::endl;

    while (!app.ShouldClose() && !source.IsClosed())
    {
        // Syncs the framebuffer to the window BEFORE commands run: a capture/burst command
        // renders + reads pixels inside Dispatch.
        app.BeginInteractiveFrame();

        // Apply every command queued since the last frame, replying to each.
        instrument::LiveItem item;
        while (source.NextItem(item))
        {
            const std::string resp = runner.Dispatch(item.cmd);
            if (item.done)
                item.done->set_value(resp);
        }

        app.PresentFrame();
    }

    server.Stop();
    std::cout << "[Live] session ended." << std::endl;
    app.Shutdown();
    return true;
}

} // namespace planets::app
