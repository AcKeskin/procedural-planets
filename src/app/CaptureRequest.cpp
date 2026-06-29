#include "CaptureRequest.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

namespace planets::app
{

namespace
{
// Parse "x,y,z" into a vec3. Returns false on any non-numeric / wrong-arity input.
bool ParseVec3(const std::string& text, glm::vec3& out)
{
    std::stringstream ss(text);
    std::string tok;
    float vals[3];
    int n = 0;
    while (std::getline(ss, tok, ',') && n < 3)
    {
        try
        {
            size_t consumed = 0;
            vals[n] = std::stof(tok, &consumed);
            if (consumed != tok.size())
                return false; // trailing garbage
        }
        catch (...)
        {
            return false;
        }
        ++n;
    }
    if (n != 3)
        return false;
    out = glm::vec3(vals[0], vals[1], vals[2]);
    return true;
}

// Fetch the value following a flag at index i; advances i. Null if missing.
const char* NextValue(int argc, char** argv, int& i, const char* flag)
{
    if (i + 1 >= argc)
    {
        std::cerr << "[Capture] " << flag << " requires a value; ignoring." << std::endl;
        return nullptr;
    }
    return argv[++i];
}

// Parse a single float; false (and leaves out untouched) on trailing garbage / non-numeric.
bool ParseFloat(const std::string& text, float& out)
{
    try
    {
        size_t consumed = 0;
        float v = std::stof(text, &consumed);
        if (consumed != text.size())
            return false;
        out = v;
        return true;
    }
    catch (...)
    {
        return false;
    }
}
} // namespace

CaptureRequest CaptureRequest::Parse(int argc, char** argv)
{
    CaptureRequest req;

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];

        if (std::strcmp(arg, "--screenshot") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--screenshot");
            if (v)
            {
                req.outputPath = v;
                req.enabled = true;
            }
        }
        else if (std::strcmp(arg, "--seed") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--seed");
            if (v)
                req.seed = static_cast<uint32_t>(std::strtoul(v, nullptr, 10));
        }
        else if (std::strcmp(arg, "--body") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--body");
            if (v)
                req.body = v;
        }
        else if (std::strcmp(arg, "--width") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--width");
            if (v)
                req.width = std::atoi(v);
        }
        else if (std::strcmp(arg, "--height") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--height");
            if (v)
                req.height = std::atoi(v);
        }
        else if (std::strcmp(arg, "--frames") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--frames");
            if (v)
                req.frames = std::atoi(v);
        }
        else if (std::strcmp(arg, "--camera") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--camera");
            if (v)
            {
                glm::vec3 cam;
                if (ParseVec3(v, cam))
                {
                    req.cameraPos = cam;
                    req.hasCamera = true;
                }
                else
                {
                    std::cerr << "[Capture] Malformed --camera '" << v << "'; using default framing."
                              << std::endl;
                }
            }
        }
        else if (std::strcmp(arg, "--distance") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--distance");
            if (v && !ParseFloat(v, req.distanceMultiplier))
                std::cerr << "[Capture] Malformed --distance '" << v << "'; keeping default." << std::endl;
        }
        else if (std::strcmp(arg, "--cinematic") == 0)
        {
            req.cinematic = true;
        }
        else if (std::strcmp(arg, "--out-pattern") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--out-pattern");
            if (v)
                req.framePattern = v;
        }
        else if (std::strcmp(arg, "--cinematic-frames") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--cinematic-frames");
            if (v)
                req.cinematicFrames = std::atoi(v);
        }
        else if (std::strcmp(arg, "--fps") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--fps");
            if (v && !ParseFloat(v, req.cinematicFps))
                std::cerr << "[Capture] Malformed --fps '" << v << "'; keeping default." << std::endl;
        }
        else if (std::strcmp(arg, "--orbit-speed") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--orbit-speed");
            if (v && !ParseFloat(v, req.orbitSpeed))
                std::cerr << "[Capture] Malformed --orbit-speed '" << v << "'; keeping default." << std::endl;
        }
        else if (std::strcmp(arg, "--start-yaw") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--start-yaw");
            if (v && !ParseFloat(v, req.startYaw))
                std::cerr << "[Capture] Malformed --start-yaw '" << v << "'; keeping default." << std::endl;
        }
        else if (std::strcmp(arg, "--start-pitch") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--start-pitch");
            if (v && !ParseFloat(v, req.startPitch))
                std::cerr << "[Capture] Malformed --start-pitch '" << v << "'; keeping default." << std::endl;
        }
        else if (std::strcmp(arg, "--light") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--light");
            if (v)
            {
                glm::vec3 dir;
                if (ParseVec3(v, dir) && glm::length(dir) > 0.0f)
                {
                    req.lightDir = dir;
                    req.hasLight = true;
                }
                else
                {
                    std::cerr << "[Capture] Malformed --light '" << v << "'; using default light." << std::endl;
                }
            }
        }
        else if (std::strcmp(arg, "--no-light-follow") == 0)
        {
            req.lightFollow = false;
        }
        else if (std::strcmp(arg, "--showcase") == 0)
        {
            req.showcase = true;
        }
        else if (std::strcmp(arg, "--planets") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--planets");
            if (v)
                req.planets = std::atoi(v);
        }
        else if (std::strcmp(arg, "--hold-frames") == 0)
        {
            const char* v = NextValue(argc, argv, i, "--hold-frames");
            if (v)
                req.holdFrames = std::atoi(v);
        }
        else
        {
            std::cerr << "[Capture] Unknown argument '" << arg << "'; ignoring." << std::endl;
        }
    }

    // --cinematic / --showcase imply headless capture mode (no --screenshot needed) and
    // write a numbered frame sequence.
    if (req.cinematic || req.showcase)
    {
        req.enabled = true;

        if (req.framePattern.empty())
            req.framePattern = "captures/frame_%04d.png";

        // The pattern must contain exactly one numeric conversion so frames get distinct names.
        if (req.framePattern.find('%') == std::string::npos)
        {
            std::cerr << "[Capture] --out-pattern '" << req.framePattern
                      << "' has no %0Nd frame number; disabling sequence capture." << std::endl;
            req.cinematic = false;
            req.showcase = false;
            req.enabled = false;
        }
    }

    if (req.enabled && req.body != "earth")
        std::cerr << "[Capture] Body '" << req.body << "' not wired; falling back to earth." << std::endl;

    return req;
}

} // namespace planets::app
