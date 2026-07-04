#include "Application.h"
#include "CaptureRequest.h"

int main(int argc, char** argv)
{
    auto request = planets::app::CaptureRequest::Parse(argc, argv);

    planets::app::Application app;

    // Live instrument: visible window + localhost command server, drive it interactively.
    if (request.live)
        return app.RunLive(request) ? 0 : -1;

    // Command-driven instrument: execute a JSON timeline (camera, live params, bursts), exit.
    if (request.script)
        return app.RunScript(request) ? 0 : -1;

    // Headless generation showcase: orbit while regenerating planets, write a sequence, exit.
    if (request.showcase)
        return app.RunGenerationShowcase(request) ? 0 : -1;

    // Headless cinematic: drive the turntable and write a numbered PNG sequence, exit.
    if (request.cinematic)
        return app.RunCinematicCapture(request) ? 0 : -1;

    // Headless capture mode: render warm-up frames, write one PNG, exit.
    if (request.enabled)
        return app.RunCapture(request) ? 0 : -1;

    if (!app.Initialize())
        return -1;

    app.Run();
    app.Shutdown();

    return 0;
}
