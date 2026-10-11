#pragma once

#include "CaptureRequest.h"

#include <memory>

namespace planets::app
{

class Application;

// One way to run the app: the interactive window, a headless capture, a script... Each mode
// boots the app, drives it to completion and shuts it down.
class RunMode
{
public:
    virtual ~RunMode() = default;

    // False on failure (the process exit code).
    virtual bool Run(Application& app) = 0;
};

// The mode the command line asks for.
std::unique_ptr<RunMode> MakeRunMode(const CaptureRequest& request);

} // namespace planets::app
