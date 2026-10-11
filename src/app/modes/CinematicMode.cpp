#include "RunModes.h"
#include "Application.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace planets::app
{

bool CinematicMode::Run(Application& app)
{
    if (!app.Boot(_request))
    {
        std::cerr << "[Cinematic] Initialization failed." << std::endl;
        return false;
    }

    // Drain the LOD so frame 1 is fully populated, not half-generated.
    if (!app.DrainGeneration(_request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Cinematic] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    // Configure the turntable from the request, then start it. Loop mode keeps the
    // controller from auto-stopping mid-sequence; the frame count defines the length.
    auto& tt = app.Turntable();
    if (_request.orbitSpeed > 0.0f)
        tt.orbitSpeed = _request.orbitSpeed;
    tt.startYaw = _request.startYaw;
    tt.startPitch = _request.startPitch;
    // --distance sets the orbit radius (multiplier of planet radius); hold it across the orbit.
    tt.zoomStartMultiplier = _request.distanceMultiplier;
    tt.zoomEndMultiplier = _request.distanceMultiplier;
    tt.durationMode = render::CinematicDurationMode::Loop;
    app.StartCinematic();

    // Fixed dt → deterministic, smooth motion independent of wall-clock.
    const float dt = 1.0f / (std::max)(1.0f, _request.cinematicFps);
    const int frameCount = (std::max)(1, _request.cinematicFrames);

    char path[512];
    int written = 0;
    for (int f = 0; f < frameCount; ++f)
    {
        app.StepCinematic(dt);
        std::snprintf(path, sizeof(path), _request.framePattern.c_str(), f + 1); // 1-based
        if (app.CaptureFrame(path))
            ++written;
    }

    app.StopCinematic();

    std::cout << "[Cinematic] Wrote " << written << "/" << frameCount << " frames." << std::endl;
    app.Shutdown();
    return written == frameCount;
}

} // namespace planets::app
