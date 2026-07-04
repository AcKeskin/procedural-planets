#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Value types for the scripted/live instrument timeline. A Command is a tagged value
// struct (no vtable) — parsed from JSON by TimelineParser, dispatched by ScriptRunner.
namespace planets::app::instrument
{

enum class CommandType
{
    SetSeed,
    SetBody,
    SetParam,
    Camera,
    Wait,
    Capture,
    Burst
};

// How a Camera command seats / drives the camera.
enum class CameraMode
{
    LookAt,  // FreeFly: eye + target
    FreeFly, // FreeFly: eye + yaw/pitch
    Orbit,   // Orbit: target + distance + yaw + pitch
    Path     // arm a waypoint path, driven by subsequent wait/burst frames
};

enum class PathEase
{
    Linear,
    Smoothstep,
    CatmullRom
};

// One key on a camera path. orbit==false → FreeFly (eye/target meaningful);
// orbit==true → Orbit (distance/yaw/pitch meaningful).
struct Waypoint
{
    float t = 0.0f; // seconds from path start
    bool orbit = false;
    glm::vec3 eye{0.0f};
    glm::vec3 target{0.0f};
    float distance = 0.0f;
    float distanceRadii = 0.0f; // >0 overrides distance: orbit at radius*distanceRadii (frames any body)
    float yaw = 0.0f;
    float pitch = 0.0f;
};

// A parameter value as it arrives from JSON; ParamRegistry coerces it to the target type.
using ParamValue = std::variant<float, int, bool, glm::vec3>;

struct SetSeedCommand
{
    uint32_t seed = 0;
};

struct SetBodyCommand
{
    std::string body; // "earth" | "volcanic" | "crystalline"
    std::string path; // optional explicit config path; wins over body if set
};

struct SetParamCommand
{
    std::string key;
    ParamValue value;
};

struct CameraCommand
{
    CameraMode mode = CameraMode::LookAt;
    glm::vec3 eye{0.0f};
    glm::vec3 target{0.0f};
    float distance = 0.0f;
    float distanceRadii = 0.0f; // >0 overrides distance: orbit at radius*distanceRadii (frames any body)
    float yaw = 0.0f;
    float pitch = 0.0f;

    // Path mode only.
    PathEase ease = PathEase::CatmullRom;
    bool loop = false;
    std::vector<Waypoint> waypoints;
};

struct WaitCommand
{
    int frames = 1; // resolved from frames or seconds at parse time
};

struct CaptureCommand
{
    std::string path;
};

struct BurstCommand
{
    int frames = 1;
    int every = 1;          // capture every Nth frame
    std::string pattern;    // printf pattern with one %0Nd
};

// Tagged union of every command payload. payload holds exactly the variant for `type`.
struct Command
{
    CommandType type = CommandType::Wait;
    std::variant<SetSeedCommand, SetBodyCommand, SetParamCommand, CameraCommand, WaitCommand, CaptureCommand, BurstCommand>
        payload;
};

// Parsed timeline: one-time setup overrides + the ordered command list.
struct Timeline
{
    // setup block (all optional; absent fields keep CaptureRequest/CLI defaults)
    bool hasSeed = false;
    uint32_t seed = 42;
    bool hasBody = false;
    std::string body = "earth";
    int width = 0;  // 0 = keep default
    int height = 0; // 0 = keep default
    float fps = 30.0f;

    std::vector<Command> commands;
};

} // namespace planets::app::instrument
