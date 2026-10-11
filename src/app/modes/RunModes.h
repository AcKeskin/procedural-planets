#pragma once

#include "RunMode.h"

namespace planets::app
{

// Visible window driven by keyboard, mouse and GUI until it closes.
class InteractiveMode final : public RunMode
{
public:
    bool Run(Application& app) override;
};

// Base for the modes that run from a CaptureRequest.
class RequestMode : public RunMode
{
public:
    explicit RequestMode(const CaptureRequest& request)
        : _request(request)
    {
    }

protected:
    CaptureRequest _request;
};

// Headless: warm up the LOD, write one PNG, exit.
class CaptureMode final : public RequestMode
{
public:
    using RequestMode::RequestMode;
    bool Run(Application& app) override;
};

// Headless: drive the turntable at a fixed dt and write a numbered PNG sequence.
class CinematicMode final : public RequestMode
{
public:
    using RequestMode::RequestMode;
    bool Run(Application& app) override;
};

// Headless: orbit while switching through the planet types, holdFrames frames each.
class ShowcaseMode final : public RequestMode
{
public:
    using RequestMode::RequestMode;
    bool Run(Application& app) override;
};

// Headless: run a JSON command timeline (camera, live params, frame bursts) deterministically.
class ScriptMode final : public RequestMode
{
public:
    using RequestMode::RequestMode;
    bool Run(Application& app) override;
};

// Visible window plus a localhost command server: applies JSON commands as they arrive
// (one per line) and renders continuously until the window closes.
class LiveMode final : public RequestMode
{
public:
    using RequestMode::RequestMode;
    bool Run(Application& app) override;
};

} // namespace planets::app
