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
        else
        {
            std::cerr << "[Capture] Unknown argument '" << arg << "'; ignoring." << std::endl;
        }
    }

    if (req.enabled && req.body != "earth")
        std::cerr << "[Capture] Body '" << req.body << "' not wired; falling back to earth." << std::endl;

    return req;
}

} // namespace planets::app
