#pragma once

#include "../settings/CinematicSettings.h"
#include "../../core/math/Camera.h"

namespace planets::render
{

enum class CinematicState
{
    Idle,
    Playing
};

// Drives camera motion during cinematic playback
class CinematicController
{
public:
    void Start(const CinematicSettings& settings, float planetRadius, core::Camera& camera);
    void Stop();
    void Update(float deltaTime, core::Camera& camera);

    CinematicState GetState() const { return _state; }
    bool IsPlaying() const { return _state == CinematicState::Playing; }
    float GetElapsedTime() const { return _elapsedTime; }
    float GetProgress() const;  // 0-1 normalized, meaningful for non-loop modes

private:
    float EaseInOut(float t) const;
    float ComputeOrbitSpeed(float progress) const;
    float ComputeZoomDistance(float progress) const;
    float ComputePitch(float elapsed) const;
    float ComputeDuration() const;

    CinematicState _state = CinematicState::Idle;
    TurntableSettings _settings;
    float _elapsedTime = 0.0f;
    float _totalDuration = 0.0f;
    float _startDistance = 0.0f;
    float _endDistance = 0.0f;
    float _currentDistance = 0.0f;
    float _lastPitch = 0.0f;
};

} // namespace planets::render
