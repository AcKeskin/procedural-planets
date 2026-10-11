#include "RunModes.h"

namespace planets::app
{

std::unique_ptr<RunMode> MakeRunMode(const CaptureRequest& request)
{
    if (request.live)
        return std::make_unique<LiveMode>(request);
    if (request.script)
        return std::make_unique<ScriptMode>(request);
    if (request.showcase)
        return std::make_unique<ShowcaseMode>(request);
    if (request.cinematic)
        return std::make_unique<CinematicMode>(request);
    if (request.enabled)
        return std::make_unique<CaptureMode>(request);
    return std::make_unique<InteractiveMode>();
}

} // namespace planets::app
