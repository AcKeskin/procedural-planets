#pragma once

#include "Command.h"

#include "../../core/math/Camera.h"

#include <vector>

namespace planets::app::instrument
{

// Drives the camera along a keyframed waypoint path over time. Standalone (not part of
// CinematicController, which is turntable-only). FreeFly waypoints interpolate eye+target
// and the camera LookAt(target)s each frame (orientation is implied, no slerp). Orbit
// waypoints interpolate distance/yaw/pitch scalars. Position/scalars use Catmull-Rom for
// >=3 keys (passes through each), lerp for 2.
class CameraPath
{
public:
    // radius resolves each orbit waypoint's distanceRadii (>0) to an absolute distance, so the
    // path frames any body identically. Waypoints keeping absolute distance ignore it.
    void Build(const CameraCommand& cmd, float radius);
    void Reset();

    bool IsArmed() const { return _armed; }
    float Duration() const { return _duration; }

    // Advance the path clock by dt and seat the camera at the new pose.
    void Advance(float dt, core::Camera& camera);

    // Seat the camera at a given absolute path time (used for testing endpoints / scrubbing).
    void SampleTo(float time, core::Camera& camera) const;

private:
    std::vector<Waypoint> _waypoints; // sorted by t
    PathEase _ease = PathEase::CatmullRom;
    bool _loop = false;
    bool _orbit = false;
    bool _armed = false;
    float _duration = 0.0f;
    float _clock = 0.0f;
};

} // namespace planets::app::instrument
