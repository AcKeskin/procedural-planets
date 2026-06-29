#pragma once

namespace planets::render
{

namespace CinematicLimits
{
inline constexpr float MinOrbitSpeed = 1.0f;
inline constexpr float MaxOrbitSpeed = 60.0f;
inline constexpr float MinPitchAmplitude = 1.0f;
inline constexpr float MaxPitchAmplitude = 30.0f;
inline constexpr float MinPitchPeriod = 1.0f;
inline constexpr float MaxPitchPeriod = 60.0f;
inline constexpr float MinZoomMultiplier = 1.01f; // just above surface
inline constexpr float MaxZoomMultiplier = 20.0f; // far orbit
inline constexpr float ZoomMultiplierStep = 0.1f;
inline constexpr float ZoomMultiplierFastStep = 1.0f;
inline constexpr float DefaultZoomMultiplier = 2.5f; // matches initial camera distance
inline constexpr float MinRotations = 1.0f;
inline constexpr float MaxRotations = 10.0f;
inline constexpr float MinDurationSeconds = 1.0f;
inline constexpr float MaxDurationSeconds = 120.0f;
inline constexpr float MinStartYaw = -180.0f;
inline constexpr float MaxStartYaw = 180.0f;
inline constexpr float MinStartPitch = -89.0f;
inline constexpr float MaxStartPitch = 89.0f;
} // namespace CinematicLimits

enum class CinematicDurationMode
{
    Rotations,
    Seconds,
    Loop
};

// Turntable cinematic: automated orbit around planet
struct TurntableSettings
{
    float orbitSpeed = 10.0f; // degrees/second
    bool speedEasing = true;  // cubic ease-in-out at boundaries
    bool pitchOscillation = false;
    float pitchAmplitude = 15.0f;                                       // degrees
    float pitchPeriod = 10.0f;                                          // seconds for one full oscillation
    float zoomStartMultiplier = CinematicLimits::DefaultZoomMultiplier; // radius multiplier
    float zoomEndMultiplier = CinematicLimits::DefaultZoomMultiplier;   // radius multiplier
    bool zoomEasing = true;
    CinematicDurationMode durationMode = CinematicDurationMode::Rotations;
    float durationValue = 1.0f; // rotations or seconds depending on mode

    // Orbit start angle. Defaults match the camera's resting yaw/pitch so default
    // behaviour (begin wherever the camera is) is unchanged.
    float startYaw = -90.0f; // matches core::CameraDefaults::Yaw
    float startPitch = 0.0f;
};

struct CinematicSettings
{
    TurntableSettings turntable;
};

} // namespace planets::render
