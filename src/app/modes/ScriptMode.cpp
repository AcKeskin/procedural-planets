#include "RunModes.h"
#include "Application.h"
#include "instrument/ScriptRunner.h"
#include "instrument/TimelineParser.h"

#include <fstream>
#include <iostream>
#include <iterator>

namespace planets::app
{

bool ScriptMode::Run(Application& app)
{
    std::ifstream in(_request.scriptPath);
    if (!in)
    {
        std::cerr << "[Script] could not open '" << _request.scriptPath << "'" << std::endl;
        return false;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::string err;
    auto timeline = instrument::ParseTimeline(text, err);
    if (!timeline)
    {
        std::cerr << "[Script] parse failed: " << err << std::endl;
        return false;
    }

    // setup block overrides the CLI/defaults before the window is built.
    CaptureRequest request = _request;
    if (timeline->hasSeed)
        request.seed = timeline->seed;
    if (timeline->hasBody)
        request.body = timeline->body;
    if (timeline->width > 0)
        request.width = timeline->width;
    if (timeline->height > 0)
        request.height = timeline->height;

    if (!app.Boot(request))
    {
        std::cerr << "[Script] initialization failed." << std::endl;
        return false;
    }

    if (timeline->hasBody && request.body != app.ActiveBodyId())
        app.SwitchBodyById(request.body);

    // Initial settle so the first command runs against a fully-generated planet.
    if (!app.DrainGeneration(request.frames, AppDefaults::CaptureDrainMaxFrames))
        std::cerr << "[Script] generation did not drain within " << AppDefaults::CaptureDrainMaxFrames
                  << " frames; starting anyway." << std::endl;

    instrument::ScriptRunner runner(app, timeline->fps);
    instrument::TimelineSource source(std::move(timeline->commands));
    runner.Run(source);

    std::cout << "[Script] timeline complete." << std::endl;
    app.Shutdown();
    return true;
}

} // namespace planets::app
