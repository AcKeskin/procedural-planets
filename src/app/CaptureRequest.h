#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

namespace planets::app
{

// Parsed --screenshot CLI request for headless capture mode.
// Empty (enabled == false) means run the normal interactive app.
struct CaptureRequest
{
    bool enabled = false;        // set when --screenshot <path> is present
    std::string outputPath;      // required; the PNG destination
    uint32_t seed = 42;
    std::string body = "earth";  // only earth is wired today
    int width = 1280;
    int height = 720;
    int frames = 10;             // warm-up frames before the shot
    bool hasCamera = false;
    glm::vec3 cameraPos{0.0f};   // eye position; looks at origin. Valid only if hasCamera

    // Parse argv. On a malformed value, logs a warning and falls back to the default
    // (malformed --camera → hasCamera stays false → default framing). Returns the request;
    // .enabled is false if --screenshot was not supplied.
    static CaptureRequest Parse(int argc, char** argv);
};

} // namespace planets::app
