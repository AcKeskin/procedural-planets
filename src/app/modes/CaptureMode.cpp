#include "RunModes.h"
#include "Application.h"

#include <iostream>

namespace planets::app
{

bool CaptureMode::Run(Application& app)
{
    if (!app.Boot(_request))
    {
        std::cerr << "[Capture] Initialization failed." << std::endl;
        return false;
    }

    // Wait for the async LOD to fully populate before the shot. --frames is the settle floor;
    // draining guarantees no half-generated patch is captured.
    if (!app.DrainGeneration(_request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Capture] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; capturing anyway." << std::endl;

    const bool ok = app.CaptureFrame();
    if (!ok)
        std::cerr << "[Capture] Capture did not run (render path skipped it)." << std::endl;

    app.Shutdown();
    return ok;
}

} // namespace planets::app
