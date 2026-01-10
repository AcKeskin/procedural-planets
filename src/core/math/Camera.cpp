#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace planets::core {

Camera::Camera()
    : _mode(CameraMode::FreeFly)
    , _position(0.0f, 0.0f, 10.0f)
    , _forward(0.0f, 0.0f, -1.0f)
    , _right(1.0f, 0.0f, 0.0f)
    , _up(0.0f, 1.0f, 0.0f)
    , _yaw(-90.0f)
    , _pitch(0.0f)
    , _orbitTarget(0.0f)
    , _orbitDistance(10.0f)
    , _fov(45.0f)
    , _nearPlane(0.1f)
    , _farPlane(1000.0f)
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

    _pitch = std::clamp(_pitch, -89.0f, 89.0f);

    UpdateVectors();
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

    _pitch = std::clamp(_pitch, -89.0f, 89.0f);

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

void Camera::OrbitZoom(float amount)
{
    _orbitDistance -= amount;
    _orbitDistance = std::max(_orbitDistance, 1.0f);

    OrbitRotate(0.0f, 0.0f);
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(_position, _position + _forward, _up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(_fov), aspectRatio, _nearPlane, _farPlane);
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
