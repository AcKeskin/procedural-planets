#include "RunModes.h"
#include "Application.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace planets::app
{

bool ShowcaseMode::Run(Application& app)
{
    if (!app.Boot(_request))
    {
        std::cerr << "[Showcase] Initialization failed." << std::endl;
        return false;
    }

    if (!app.DrainGeneration(_request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Showcase] Generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    auto& tt = app.Turntable();
    if (_request.orbitSpeed > 0.0f)
        tt.orbitSpeed = _request.orbitSpeed;
    tt.zoomStartMultiplier = _request.distanceMultiplier;
    tt.zoomEndMultiplier = _request.distanceMultiplier;
    tt.durationMode = render::CinematicDurationMode::Loop;

    const float dt = 1.0f / (std::max)(1.0f, _request.cinematicFps);
    const int planets = (std::max)(1, _request.planets);
    const int hold = (std::max)(1, _request.holdFrames);

    // Cycle through every planet type for visual contrast.
    const auto& bodies = app.Bodies();
    const int typeCount = (std::max)(1, static_cast<int>(bodies.size()));

    char path[512];
    int written = 0;
    int frameIndex = 0;
    for (int p = 0; p < planets; ++p)
    {
        // Each segment: switch to the next body type. SwitchBody resets the camera + radius,
        // so re-seat the orbit afterwards.
        const int type = p % typeCount;
        // Earth segments use the authored earth.json with a stepped seed (reliably
        // land-facing, like the hero still). Other types roll their variety from the seed.
        // Random earths placed their land on the back/poles too often to use here.
        if (type == 0)
            app.SetSeed(_request.seed + static_cast<uint32_t>(p));
        app.SwitchBodyById(bodies.empty() ? "earth" : bodies[type].id);

        app.ApplyCaptureExposure();
        app.StartCinematic(); // re-seat orbit target/distance for the new radius
        app.DrainGeneration(1, AppDefaults::CaptureDrainMaxFrames);

        for (int h = 0; h < hold; ++h, ++frameIndex)
        {
            app.StepCinematic(dt);
            std::snprintf(path, sizeof(path), _request.framePattern.c_str(), frameIndex + 1);
            if (app.CaptureFrame(path))
                ++written;
        }
    }

    app.StopCinematic();

    const int total = planets * hold;
    std::cout << "[Showcase] Wrote " << written << "/" << total << " frames across " << planets << " planets."
              << std::endl;
    app.Shutdown();
    return written == total;
}

} // namespace planets::app
