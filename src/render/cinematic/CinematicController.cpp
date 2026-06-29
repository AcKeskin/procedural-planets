#include "CinematicController.h"
#include "../../core/math/Constants.h"
#include <cmath>
#include <algorithm>

namespace planets::render
{

void CinematicController::Start(const CinematicSettings& settings, float planetRadius, core::Camera& camera)
{
    _state = CinematicState::Playing;
    _settings = settings.turntable;
    _elapsedTime = 0.0f;
    _totalDuration = ComputeDuration();

    // Convert radius-relative multipliers to absolute distances
    _startDistance = _settings.zoomStartMultiplier * planetRadius;
    _endDistance = _settings.zoomEndMultiplier * planetRadius;
    _currentDistance = _startDistance;

    // Seat the camera at the chosen start angle facing the planet; _lastPitch tracks the
    // seated pitch so the first frame's pitch delta in Update() doesn't snap.
    _lastPitch = _settings.startPitch;

    camera.SetOrbitDistance(_startDistance);
    camera.SetOrbitAngles(_settings.startYaw, _settings.startPitch);
}

void CinematicController::Stop()
{
    _state = CinematicState::Idle;
}

void CinematicController::Update(float deltaTime, core::Camera& camera)
{
    if (_state != CinematicState::Playing)
        return;

    _elapsedTime += deltaTime;

    // Check completion for non-loop modes
    if (_settings.durationMode != CinematicDurationMode::Loop && _elapsedTime >= _totalDuration)
    {
        Stop();
        return;
    }

    const float progress = GetProgress();
    const float orbitSpeed = ComputeOrbitSpeed(progress);
    const float pitch = ComputePitch(_elapsedTime);
    const float targetDistance = ComputeZoomDistance(progress);

    // Apply orbit rotation
    const float yawDelta = orbitSpeed * deltaTime;
    camera.OrbitRotate(yawDelta, 0.0f);

    // Apply pitch oscillation as delta from previous frame
    const float pitchDelta = pitch - _lastPitch;
    camera.OrbitRotate(0.0f, pitchDelta);
    _lastPitch = pitch;

    // Apply zoom
    const float zoomDelta = targetDistance - _currentDistance;
    camera.OrbitZoom(-zoomDelta); // negative because OrbitZoom subtracts
    _currentDistance = targetDistance;
}

float CinematicController::GetProgress() const
{
    if (_settings.durationMode == CinematicDurationMode::Loop)
    {
        // Normalized progress within one loop cycle
        const float loopDuration = core::DegreesPerRotation / _settings.orbitSpeed;
        return std::fmod(_elapsedTime, loopDuration) / loopDuration;
    }

    return std::min(_elapsedTime / _totalDuration, 1.0f);
}

float CinematicController::EaseInOut(float t) const
{
    // Cubic ease-in-out
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float CinematicController::ComputeOrbitSpeed(float progress) const
{
    if (!_settings.speedEasing)
        return _settings.orbitSpeed;

    // Speed modulation creates acceleration/deceleration feel
    // Peaks at middle (progress=0.5), slower at boundaries
    // Minimum 10% speed at boundaries prevents camera stalling at start/end
    constexpr float minSpeedFraction = 0.1f;
    float speedCurve = 1.0f - std::abs(2.0f * progress - 1.0f);
    return _settings.orbitSpeed * std::max(speedCurve, minSpeedFraction);
}

float CinematicController::ComputeZoomDistance(float progress) const
{
    const float t = _settings.zoomEasing ? EaseInOut(progress) : progress;
    return _startDistance + t * (_endDistance - _startDistance);
}

float CinematicController::ComputePitch(float elapsed) const
{
    if (!_settings.pitchOscillation)
        return 0.0f;

    return _settings.pitchAmplitude * std::sin(core::TwoPi * elapsed / _settings.pitchPeriod);
}

float CinematicController::ComputeDuration() const
{
    switch (_settings.durationMode)
    {
    case CinematicDurationMode::Rotations:
        // Total time = (degrees per rotation / degrees per second) * number of rotations
        return (core::DegreesPerRotation / _settings.orbitSpeed) * _settings.durationValue;

    case CinematicDurationMode::Seconds:
        return _settings.durationValue;

    case CinematicDurationMode::Loop:
        return 0.0f; // infinite

    default:
        return 0.0f;
    }
}

} // namespace planets::render
