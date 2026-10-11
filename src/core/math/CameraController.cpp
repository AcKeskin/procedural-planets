#include "CameraController.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace planets::core
{

namespace
{
constexpr float ClearanceRadii = 0.01f;     // margin above the floor, in planet radii
constexpr float MaxDistanceRadii = 30.0f;   // orbit zoom-out limit
constexpr float FrameFill = 0.8f;           // F: the planet and its glow use this share of the view height
constexpr float FrameGlowRadii = 1.08f;     // the atmosphere glow reaches about this far
constexpr float LevelFullRadii = 1.5f;      // fully level below this distance from the centre
constexpr float LevelNoneRadii = 4.0f;      // not levelled beyond this
constexpr float ZoomNotchFraction = 0.85f;  // each notch keeps this share of the altitude
constexpr float ZoomEaseRate = 10.0f;
constexpr float VelocityEaseRate = 8.0f;
constexpr float OrbitDegreesPerPixel = 0.3f;
constexpr float LookDegreesPerPixel = 0.1f;
constexpr float SpeedNotchFactor = 1.15f;
constexpr float MinAltitudeRadii = 0.02f; // fly speed stops shrinking below this altitude
constexpr float MinSpeedRadii = 0.002f;
constexpr float MaxSpeedRadii = 30.0f;
constexpr float MoveSeconds = 1.2f;
constexpr float ModeSwitchSeconds = 0.7f;
constexpr float FlyToSeconds = 1.6f;
constexpr float FlyToAltitudeRadii = 0.03f; // above the floor, so the arrival is near the ground

const glm::vec3 WorldUp(0.0f, 1.0f, 0.0f);

float SmootherStep(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& fallback)
{
    const float len2 = glm::dot(v, v);
    return len2 > 1e-12f ? v / std::sqrt(len2) : fallback;
}

// Vertical for the fly camera: world up far away, the surface normal near the planet.
glm::vec3 LocalUp(const glm::vec3& position, float radius)
{
    const float dist = glm::length(position);
    const float t = std::clamp((dist - LevelFullRadii * radius) / ((LevelNoneRadii - LevelFullRadii) * radius), 0.0f, 1.0f);
    const float blend = 1.0f - t * t * (3.0f - 2.0f * t);
    return SafeNormalize(glm::mix(WorldUp, SafeNormalize(position, WorldUp), blend), WorldUp);
}

glm::vec3 ClampAboveFloor(const glm::vec3& position, const CameraBody& body)
{
    const float minRadius = body.floorRadius + body.radius * ClearanceRadii;
    const float dist = glm::length(position);
    if (dist >= minRadius)
        return position;
    return dist > 1e-6f ? position * (minRadius / dist) : glm::vec3(0.0f, 0.0f, minRadius);
}

// Looking at the planet centre from `position`, world up.
CameraPose OrbitPoseAt(const glm::vec3& position)
{
    CameraPose pose;
    pose.mode = CameraMode::Orbit;
    pose.position = position;
    pose.forward = SafeNormalize(-position, glm::vec3(0.0f, 0.0f, -1.0f));
    const glm::vec3 right = SafeNormalize(glm::cross(pose.forward, WorldUp), glm::vec3(1.0f, 0.0f, 0.0f));
    pose.up = glm::cross(right, pose.forward);
    return pose;
}

glm::quat BasisToQuat(const glm::vec3& forward, const glm::vec3& up)
{
    const glm::vec3 right = SafeNormalize(glm::cross(forward, up), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 trueUp = glm::cross(right, forward);
    return glm::quat_cast(glm::mat3(right, trueUp, -forward));
}
} // namespace

void CameraController::SyncOrbit(Camera& camera, const glm::vec3& target) const
{
    camera.SetMode(CameraMode::Orbit);
    camera.SetOrbitTarget(target); // takes the distance from the current position
    const glm::vec3 offset = camera.GetPosition() - target;
    const float dist = glm::length(offset);
    if (dist < 1e-6f)
        return;
    camera.SetOrbitAngles(glm::degrees(std::atan2(offset.z, offset.x)),
                          glm::degrees(std::asin(std::clamp(offset.y / dist, -1.0f, 1.0f))));
}

CameraPose CameraController::CapturePose(const Camera& camera) const
{
    return {camera.GetPosition(), camera.GetForward(), camera.GetUp(), camera.GetMode()};
}

void CameraController::Cancel(Camera& camera)
{
    if (_move.active)
        EndMove(camera, /*snapToTarget=*/false);
    _velocity = glm::vec3(0.0f);
    _zoomValid = false;
    _haveLast = false;
}

void CameraController::Update(float dt, const CameraBody& body, const CameraControl& input, Camera& camera)
{
    _bodyRadius = body.radius;
    // Somebody else moved the camera (script, turntable, body switch): adopt that pose.
    const bool moved = _haveLast && glm::distance(camera.GetPosition(), _lastPosition) >
                                        1e-5f * (1.0f + glm::length(camera.GetPosition()));
    if (moved)
        _move.active = false;
    if ((!_haveLast || moved) && !_move.active)
    {
        _velocity = glm::vec3(0.0f);
        _zoomValid = false;
        if (camera.GetMode() == CameraMode::Orbit)
            SyncOrbit(camera, camera.GetOrbitTarget());
    }

    const bool userActs = input.move != glm::vec3(0.0f) || input.zoomNotches != 0.0f ||
                          input.orbitDrag != glm::vec2(0.0f) || input.look != glm::vec2(0.0f);
    if (_move.active && userActs)
        EndMove(camera, /*snapToTarget=*/false);

    // WASD/QE or a right-drag in Orbit means "fly": switch at once instead of ignoring the keys.
    const bool wantsFly = input.move != glm::vec3(0.0f) || input.look != glm::vec2(0.0f);
    if (wantsFly && !_move.active && camera.GetMode() == CameraMode::Orbit)
    {
        camera.SetMode(CameraMode::FreeFly);
        camera.Relevel(LocalUp(camera.GetPosition(), body.radius));
        camera.Hud().autoRotate = false;
    }

    if (_move.active)
        StepMove(dt, camera);
    else if (camera.GetMode() == CameraMode::Orbit)
        UpdateOrbit(dt, body, input, camera);
    else
        UpdateFly(dt, body, input, camera);

    UpdateHud(body, input, camera);
    _lastPosition = camera.GetPosition();
    _haveLast = true;
}

void CameraController::UpdateOrbit(float dt, const CameraBody& body, const CameraControl& input, Camera& camera)
{
    const float distance = camera.GetOrbitDistance();
    const float minDistance = body.floorRadius + body.radius * ClearanceRadii;
    const float maxDistance = body.radius * MaxDistanceRadii;

    if (!_zoomValid)
    {
        _zoomTarget = distance;
        _zoomValid = true;
    }

    // Log steps: each notch takes a fraction of the altitude, so zoom stays usable at any height.
    if (input.zoomNotches != 0.0f)
    {
        const float altitude = (std::max)(_zoomTarget - body.floorRadius, 1e-4f * body.radius);
        _zoomTarget = std::clamp(body.floorRadius + altitude * std::pow(ZoomNotchFraction, input.zoomNotches),
                                 minDistance, maxDistance);
    }

    // Finer drag near the ground: the same pixels cover less surface.
    const float dragScale =
        OrbitDegreesPerPixel * std::clamp((distance - body.floorRadius) / body.radius, 0.05f, 1.0f);
    float yaw = -input.orbitDrag.x * dragScale;
    const float pitch = -input.orbitDrag.y * dragScale;
    if (camera.Hud().autoRotate)
        yaw += input.autoRotateDegPerSec * dt;
    if (yaw != 0.0f || pitch != 0.0f)
        camera.OrbitTumble(yaw, pitch);

    if (std::abs(_zoomTarget - distance) > 1e-4f * body.radius)
    {
        const float eased = distance + (_zoomTarget - distance) * (1.0f - std::exp(-ZoomEaseRate * dt));
        camera.SetOrbitDistance(eased);
    }
}

float CameraController::FlySpeedAt(const CameraBody& body, const glm::vec3& position) const
{
    // Proportional to altitude, so the ground passes at a readable pace from any height.
    const float altitude = glm::length(position) - body.seaRadius;
    return std::clamp((std::max)(altitude, body.radius * MinAltitudeRadii) * _speedMultiplier,
                      body.radius * MinSpeedRadii, body.radius * MaxSpeedRadii);
}

void CameraController::UpdateFly(float dt, const CameraBody& body, const CameraControl& input, Camera& camera)
{
    if (input.zoomNotches != 0.0f)
        _speedMultiplier = std::clamp(_speedMultiplier * std::pow(SpeedNotchFactor, input.zoomNotches),
                                      MinSpeedMultiplier, MaxSpeedMultiplier);

    glm::vec3 position = camera.GetPosition();
    const float flySpeed = FlySpeedAt(body, position);

    const glm::vec3 up = LocalUp(position, body.radius);
    if (input.look != glm::vec2(0.0f))
        camera.Look(input.look.x * LookDegreesPerPixel, input.look.y * LookDegreesPerPixel, up);

    glm::vec3 wish = camera.GetRight() * input.move.x + up * input.move.y + camera.GetForward() * input.move.z;
    if (glm::dot(wish, wish) > 1.0f)
        wish = glm::normalize(wish);
    const glm::vec3 target = wish * flySpeed * input.boost;

    _velocity += (target - _velocity) * (1.0f - std::exp(-VelocityEaseRate * dt));
    if (target == glm::vec3(0.0f) && glm::dot(_velocity, _velocity) < 1e-8f * flySpeed * flySpeed)
        _velocity = glm::vec3(0.0f);

    if (_velocity != glm::vec3(0.0f))
    {
        position = ClampAboveFloor(position + _velocity * dt, body);
        camera.SetPosition(position);
        camera.Relevel(LocalUp(position, body.radius));
    }
}

void CameraController::UpdateHud(const CameraBody& body, const CameraControl& input, Camera& camera) const
{
    CameraHud& hud = camera.Hud();
    hud.altitude = glm::length(camera.GetPosition()) - body.seaRadius;
    hud.speed = FlySpeedAt(body, camera.GetPosition()) * input.boost;
}

void CameraController::BeginMove(const CameraPose& to, float duration, Camera& camera)
{
    if (_move.active)
        EndMove(camera, /*snapToTarget=*/false);
    _move.active = true;
    _move.t = 0.0f;
    _move.duration = (std::max)(duration, 0.05f);
    _move.from = CapturePose(camera);
    _move.to = to;
    _velocity = glm::vec3(0.0f);
}

void CameraController::StepMove(float dt, Camera& camera)
{
    _move.t = (std::min)(_move.t + dt / _move.duration, 1.0f);
    const float e = SmootherStep(_move.t);

    // Swing around the planet and ease the radius in log space, so the path never cuts through it.
    const glm::vec3 a = _move.from.position;
    const glm::vec3 b = _move.to.position;
    const float ra = (std::max)(glm::length(a), 1e-4f);
    const float rb = (std::max)(glm::length(b), 1e-4f);
    const glm::vec3 da = a / ra;
    const glm::vec3 db = b / rb;
    const float angle = std::acos(std::clamp(glm::dot(da, db), -1.0f, 1.0f));
    glm::vec3 axis = glm::cross(da, db);
    if (glm::dot(axis, axis) < 1e-10f)
        axis = SafeNormalize(glm::cross(da, WorldUp), glm::vec3(1.0f, 0.0f, 0.0f));
    else
        axis = glm::normalize(axis);
    const glm::vec3 dir = glm::angleAxis(angle * e, axis) * da;
    const float radius = std::exp(glm::mix(std::log(ra), std::log(rb), e));

    const glm::quat q = glm::slerp(BasisToQuat(_move.from.forward, _move.from.up),
                                   BasisToQuat(_move.to.forward, _move.to.up), e);
    const glm::mat3 basis = glm::mat3_cast(q);

    camera.SetPosition(dir * radius);
    camera.SetOrientation(-basis[2], basis[1]);

    if (_move.t >= 1.0f)
        EndMove(camera, /*snapToTarget=*/true);
}

void CameraController::EndMove(Camera& camera, bool snapToTarget)
{
    const CameraPose& to = _move.to;
    _move.active = false;
    _velocity = glm::vec3(0.0f);
    _zoomValid = false;

    if (snapToTarget)
        camera.SetPosition(to.position);

    if (to.mode == CameraMode::Orbit)
    {
        SyncOrbit(camera, glm::vec3(0.0f)); // recomputes orientation: centre in view, world up
    }
    else
    {
        camera.SetMode(CameraMode::FreeFly);
        if (snapToTarget)
            camera.SetOrientation(to.forward, to.up);
        else
            camera.Relevel(LocalUp(camera.GetPosition(), _bodyRadius));
    }
    _lastPosition = camera.GetPosition();
}

void CameraController::ToggleMode(const CameraBody& body, Camera& camera)
{
    _bodyRadius = body.radius;
    if (_move.active)
        EndMove(camera, /*snapToTarget=*/false);
    _velocity = glm::vec3(0.0f);

    if (camera.GetMode() == CameraMode::Orbit)
    {
        camera.SetMode(CameraMode::FreeFly);
        camera.Relevel(LocalUp(camera.GetPosition(), body.radius));
    }
    else
    {
        BeginMove(OrbitPoseAt(ClampAboveFloor(camera.GetPosition(), body)), ModeSwitchSeconds, camera);
    }
}

void CameraController::FramePlanet(const CameraBody& body, Camera& camera)
{
    _bodyRadius = body.radius;
    // Keep the side we are on; stay off the exact poles, where the orbit up vector flips.
    glm::vec3 dir = SafeNormalize(camera.GetPosition(), glm::vec3(0.0f, 0.0f, 1.0f));
    dir.y = std::clamp(dir.y, -0.98f, 0.98f);
    dir = glm::normalize(dir);
    // Distance from the field of view, so the whole planet fits at any FOV.
    const float halfView = glm::radians(camera.GetFov()) * 0.5f * FrameFill;
    const float distance = body.radius * FrameGlowRadii / std::sin(halfView);
    BeginMove(OrbitPoseAt(dir * distance), MoveSeconds, camera);
}

void CameraController::ResetView(const CameraBody& body, Camera& camera, bool eased)
{
    _bodyRadius = body.radius;
    const glm::vec3 start(0.0f, 0.0f, body.startDistance);
    if (eased)
    {
        BeginMove(OrbitPoseAt(start), MoveSeconds, camera);
        return;
    }
    _move.active = false;
    _velocity = glm::vec3(0.0f);
    _zoomValid = false;
    camera.SetPosition(start);
    SyncOrbit(camera, glm::vec3(0.0f));
    _lastPosition = camera.GetPosition();
    _haveLast = true;
}

void CameraController::FlyToPoint(const glm::vec3& surfacePoint, const CameraBody& body, Camera& camera)
{
    _bodyRadius = body.radius;
    const glm::vec3 normal = SafeNormalize(surfacePoint, WorldUp);

    // Face the horizon in the direction we were already looking.
    glm::vec3 forward = camera.GetForward() - normal * glm::dot(camera.GetForward(), normal);
    forward = SafeNormalize(forward, SafeNormalize(glm::cross(WorldUp, normal), glm::vec3(1.0f, 0.0f, 0.0f)));

    CameraPose pose;
    pose.mode = CameraMode::FreeFly;
    pose.position = normal * (body.floorRadius + body.radius * (ClearanceRadii + FlyToAltitudeRadii));
    pose.forward = forward;
    pose.up = normal;
    BeginMove(pose, FlyToSeconds, camera);
}

void CameraController::GoTo(const CameraPose& pose, Camera& camera)
{
    BeginMove(pose, MoveSeconds, camera);
}

bool CameraController::RaySphere(const glm::vec3& origin, const glm::vec3& dir, float radius, glm::vec3& hit)
{
    const float b = glm::dot(origin, dir);
    const float c = glm::dot(origin, origin) - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0f)
        return false;
    const float root = std::sqrt(disc);
    float t = -b - root;
    if (t < 0.0f)
        t = -b + root;
    if (t < 0.0f)
        return false;
    hit = origin + dir * t;
    return true;
}

glm::vec3 CameraController::ScreenRay(const Camera& camera, float ndcX, float ndcY, float aspect)
{
    const float halfHeight = std::tan(glm::radians(camera.GetFov()) * 0.5f);
    return glm::normalize(camera.GetForward() + camera.GetRight() * (ndcX * halfHeight * aspect) +
                          camera.GetUp() * (ndcY * halfHeight));
}

} // namespace planets::core
