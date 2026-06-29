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
    glm::vec3 cameraPos{0.0f};   // explicit eye position; looks at origin. Valid only if hasCamera

    // Default framing (no --camera): camera sits on the light axis at
    // normalize(lightDir) * radius * distanceMultiplier, looking at origin, so the
    // fully-lit hemisphere faces the camera.
    float distanceMultiplier = 2.2f;

    // --- Headless cinematic sequence (--cinematic) ---
    bool cinematic = false;        // drive the turntable and write a numbered PNG sequence
    std::string framePattern;      // printf pattern with one %0Nd, e.g. out/frame_%04d.png
    int cinematicFrames = 120;     // frames to render over the orbit
    float cinematicFps = 30.0f;    // fixed dt = 1/fps → deterministic motion
    float orbitSpeed = 0.0f;       // deg/sec; 0 = use TurntableSettings default
    float startYaw = -90.0f;       // orbit start angle (matches CameraDefaults::Yaw)
    float startPitch = 0.0f;

    // Parse argv. On a malformed value, logs a warning and falls back to the default
    // (malformed --camera → hasCamera stays false → default framing). Returns the request;
    // .enabled is false if --screenshot was not supplied.
    static CaptureRequest Parse(int argc, char** argv);
};

} // namespace planets::app
