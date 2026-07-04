#include "CameraPath.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/spline.hpp>

#include <algorithm>

namespace planets::app::instrument
{
namespace
{

float Ease(PathEase e, float t)
{
    switch (e)
    {
    case PathEase::Linear:
        return t;
    case PathEase::Smoothstep:
        return t * t * (3.0f - 2.0f * t);
    case PathEase::CatmullRom:
    default:
        return t; // segment param stays linear; the spline itself provides the curve
    }
}

// Catmull-Rom needs a point before and after the segment; clamp at the ends.
template <typename T> T CatmullSegment(const std::vector<T>& pts, int i, float u)
{
    const int n = static_cast<int>(pts.size());
    const T& p0 = pts[(std::max)(i - 1, 0)];
    const T& p1 = pts[i];
    const T& p2 = pts[(std::min)(i + 1, n - 1)];
    const T& p3 = pts[(std::min)(i + 2, n - 1)];
    return glm::catmullRom(p0, p1, p2, p3, u);
}

} // namespace

void CameraPath::Build(const CameraCommand& cmd, float radius)
{
    Reset();
    if (cmd.waypoints.empty())
        return;

    _waypoints = cmd.waypoints;
    // Bake radii-relative distances to absolute now, so sampling stays radius-unaware.
    for (Waypoint& w : _waypoints)
        if (w.orbit && w.distanceRadii > 0.0f)
            w.distance = radius * w.distanceRadii;
    std::stable_sort(
        _waypoints.begin(), _waypoints.end(), [](const Waypoint& a, const Waypoint& b) { return a.t < b.t; });
    _ease = cmd.ease;
    _loop = cmd.loop;
    _orbit = _waypoints.front().orbit;
    _duration = _waypoints.back().t;
    _clock = 0.0f;
    _armed = true;
}

void CameraPath::Reset()
{
    _waypoints.clear();
    _armed = false;
    _duration = 0.0f;
    _clock = 0.0f;
}

void CameraPath::Advance(float dt, core::Camera& camera)
{
    if (!_armed)
        return;
    _clock += dt;
    if (_loop && _duration > 0.0f)
        _clock = std::fmod(_clock, _duration);
    SampleTo(_clock, camera);
}

void CameraPath::SampleTo(float time, core::Camera& camera) const
{
    if (_waypoints.empty())
        return;

    const int n = static_cast<int>(_waypoints.size());
    const float clamped = (std::min)((std::max)(time, _waypoints.front().t), _waypoints.back().t);

    // single waypoint → static pose
    if (n == 1)
    {
        const Waypoint& w = _waypoints[0];
        if (_orbit)
        {
            camera.SetMode(core::CameraMode::Orbit);
            camera.SetOrbitTarget(w.target);
            camera.SetOrbitDistance(w.distance);
            camera.SetOrbitAngles(w.yaw, w.pitch);
        }
        else
        {
            camera.SetMode(core::CameraMode::FreeFly);
            camera.SetPosition(w.eye);
            camera.LookAt(w.target);
        }
        return;
    }

    // find the active segment [i, i+1]
    int i = 0;
    while (i < n - 2 && clamped > _waypoints[i + 1].t)
        ++i;

    const Waypoint& a = _waypoints[i];
    const Waypoint& b = _waypoints[i + 1];
    const float span = (b.t - a.t);
    float u = (span > 1e-6f) ? (clamped - a.t) / span : 0.0f;
    u = Ease(_ease, u);

    if (_orbit)
    {
        const float dist = glm::mix(a.distance, b.distance, u);
        const float yaw = glm::mix(a.yaw, b.yaw, u);
        const float pitch = glm::mix(a.pitch, b.pitch, u);
        camera.SetMode(core::CameraMode::Orbit);
        camera.SetOrbitTarget(glm::mix(a.target, b.target, u));
        camera.SetOrbitDistance(dist);
        camera.SetOrbitAngles(yaw, pitch);
        return;
    }

    glm::vec3 eye;
    glm::vec3 target;
    if (_ease == PathEase::CatmullRom && n >= 3)
    {
        std::vector<glm::vec3> eyes, targets;
        eyes.reserve(n);
        targets.reserve(n);
        for (const auto& w : _waypoints)
        {
            eyes.push_back(w.eye);
            targets.push_back(w.target);
        }
        eye = CatmullSegment(eyes, i, u);
        target = CatmullSegment(targets, i, u);
    }
    else
    {
        eye = glm::mix(a.eye, b.eye, u);
        target = glm::mix(a.target, b.target, u);
    }

    camera.SetMode(core::CameraMode::FreeFly);
    camera.SetPosition(eye);
    camera.LookAt(target);
}

} // namespace planets::app::instrument
