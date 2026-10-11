#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace planets::core
{

Camera::Camera()
    : _mode(CameraMode::FreeFly)
    , _position(0.0f, 0.0f, 10.0f)
    , _forward(0.0f, 0.0f, -1.0f)
    , _right(1.0f, 0.0f, 0.0f)
    , _up(0.0f, 1.0f, 0.0f)
    , _yaw(CameraDefaults::Yaw)
    , _pitch(CameraDefaults::Pitch)
    , _orbitTarget(0.0f)
    , _orbitDistance(CameraDefaults::OrbitDistance)
    , _fov(CameraDefaults::Fov)
    , _nearPlane(CameraDefaults::NearPlane)
    , _farPlane(CameraDefaults::FarPlane)
{
    UpdateVectors();
}

void Camera::MoveForward(float amount)
{
    _position += _forward * amount;
}

void Camera::MoveRight(float amount)
{
    _position += _right * amount;
}

void Camera::MoveUp(float amount)
{
    _position += _up * amount;
}

void Camera::Rotate(float yawDelta, float pitchDelta)
{
    _yaw += yawDelta;
    _pitch += pitchDelta;

    _pitch = std::clamp(_pitch, -CameraDefaults::MaxPitch, CameraDefaults::MaxPitch);

    UpdateVectors();
}

void Camera::LookAt(const glm::vec3& target)
{
    _forward = glm::normalize(target - _position);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    _right = glm::normalize(glm::cross(_forward, worldUp));
    _up = glm::normalize(glm::cross(_right, _forward));

    // Back-solve the Euler state so a later delta Rotate continues smoothly from here.
    _pitch = glm::degrees(std::asin(std::clamp(_forward.y, -1.0f, 1.0f)));
    _yaw = glm::degrees(std::atan2(_forward.z, _forward.x));
}

void Camera::SetOrientation(const glm::vec3& forward, const glm::vec3& upHint)
{
    _forward = glm::normalize(forward);

    glm::vec3 right = glm::cross(_forward, upHint);
    if (glm::dot(right, right) < 1e-8f)
        right = _right; // looking along the hint: keep the old roll
    _right = glm::normalize(right);
    _up = glm::normalize(glm::cross(_right, _forward));

    _pitch = glm::degrees(std::asin(std::clamp(_forward.y, -1.0f, 1.0f)));
    _yaw = glm::degrees(std::atan2(_forward.z, _forward.x));
}

void Camera::Look(float yawDelta, float pitchDelta, const glm::vec3& up)
{
    const glm::vec3 vertical = glm::normalize(up);

    // Yaw (positive = turn right, same sense as Rotate) about the vertical.
    glm::vec3 forward = glm::angleAxis(glm::radians(-yawDelta), vertical) * _forward;

    const float current = glm::degrees(std::asin(std::clamp(glm::dot(forward, vertical), -1.0f, 1.0f)));
    const float wanted = std::clamp(current + pitchDelta, -CameraDefaults::MaxPitch, CameraDefaults::MaxPitch);
    const glm::vec3 right = glm::cross(forward, vertical);
    if (glm::dot(right, right) > 1e-8f)
        forward = glm::angleAxis(glm::radians(wanted - current), glm::normalize(right)) * forward;

    SetOrientation(forward, vertical);
}

void Camera::Relevel(const glm::vec3& up)
{
    SetOrientation(_forward, up);
}

void Camera::SetOrbitTarget(const glm::vec3& target)
{
    _orbitTarget = target;
    _orbitDistance = glm::length(_position - target);
}

void Camera::OrbitRotate(float yawDelta, float pitchDelta)
{
    _yaw += yawDelta;
    _pitch += pitchDelta;

    _pitch = std::clamp(_pitch, -CameraDefaults::MaxPitch, CameraDefaults::MaxPitch);

    float yawRad = glm::radians(_yaw);
    float pitchRad = glm::radians(_pitch);

    _position.x = _orbitTarget.x + _orbitDistance * cos(pitchRad) * cos(yawRad);
    _position.y = _orbitTarget.y + _orbitDistance * sin(pitchRad);
    _position.z = _orbitTarget.z + _orbitDistance * cos(pitchRad) * sin(yawRad);

    _forward = glm::normalize(_orbitTarget - _position);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    _right = glm::normalize(glm::cross(_forward, worldUp));
    _up = glm::normalize(glm::cross(_right, _forward));
}

void Camera::OrbitTumble(float yawDelta, float pitchDelta)
{
    // Negative angles match OrbitRotate: +yaw moves from +x toward +z, +pitch moves up.
    const glm::quat turn =
        glm::angleAxis(glm::radians(-yawDelta), _up) * glm::angleAxis(glm::radians(-pitchDelta), _right);
    const glm::vec3 offset = turn * (_position - _orbitTarget);
    _position = _orbitTarget + offset;
    _forward = glm::normalize(-offset);
    _right = glm::normalize(glm::cross(_forward, turn * _up));
    _up = glm::cross(_right, _forward);

    // Keep yaw/pitch readable for the HUD and for code that seats angles later.
    const float dist = glm::length(offset);
    _yaw = glm::degrees(std::atan2(offset.z, offset.x));
    _pitch = glm::degrees(std::asin(std::clamp(offset.y / dist, -1.0f, 1.0f)));
}

void Camera::OrbitZoom(float amount)
{
    SetOrbitDistance(_orbitDistance - amount);
}

void Camera::SetOrbitDistance(float distance)
{
    // Moves along the current direction and keeps the orientation, so a tumbled view stays as it is.
    _orbitDistance = std::max(distance, CameraDefaults::MinOrbitDistance);
    const glm::vec3 offset = _position - _orbitTarget;
    const float len = glm::length(offset);
    if (len < 1e-6f)
    {
        OrbitRotate(0.0f, 0.0f);
        return;
    }
    _position = _orbitTarget + offset * (_orbitDistance / len);
}

void Camera::SetOrbitAngles(float yaw, float pitch)
{
    _yaw = yaw;
    _pitch = std::clamp(pitch, -CameraDefaults::MaxPitch, CameraDefaults::MaxPitch);
    OrbitRotate(0.0f, 0.0f); // recompute position + forward from the seated angles
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(_position, _position + _forward, _up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    // Reversed-Z: map far->0, near->1 so float depth spreads uniformly across the
    // range (kills far-plane z-fighting at large radius). Two required pieces:
    //   1. a [0,1] clip-depth projection (perspectiveZO, not the default [-1,1]) — the
    //      renderer sets glClipControl(..., GL_ZERO_TO_ONE) to match;
    //   2. swap near/far so the mapping is inverted.
    // Paired with GL_GREATER depth test + clear-depth 0.0 in the renderer.
    return glm::perspectiveZO(glm::radians(_fov), aspectRatio, _farPlane, _nearPlane);
}

void Camera::UpdateVectors()
{
    float yawRad = glm::radians(_yaw);
    float pitchRad = glm::radians(_pitch);

    _forward.x = cos(yawRad) * cos(pitchRad);
    _forward.y = sin(pitchRad);
    _forward.z = sin(yawRad) * cos(pitchRad);
    _forward = glm::normalize(_forward);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    _right = glm::normalize(glm::cross(_forward, worldUp));
    _up = glm::normalize(glm::cross(_right, _forward));
}

} // namespace planets::core
