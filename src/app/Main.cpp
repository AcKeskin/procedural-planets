#include "Application.h"
#include "CaptureRequest.h"

int main(int argc, char** argv)
{
    auto request = planets::app::CaptureRequest::Parse(argc, argv);

    planets::app::Application app;

    // Headless capture mode: render warm-up frames, write one PNG, exit.
    if (request.enabled)
        return app.RunCapture(request) ? 0 : -1;

    if (!app.Initialize())
        return -1;

    app.Run();
    app.Shutdown();

    return 0;
}
