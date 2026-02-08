#pragma once

#include <glm/glm.hpp>

namespace planets::core
{

enum class CameraMode
{
    FreeFly,
    Orbit
};

namespace CameraDefaults
{
    inline constexpr float Yaw = -90.0f;
    inline constexpr float Pitch = 0.0f;
    inline constexpr float OrbitDistance = 10.0f;
    inline constexpr float Fov = 45.0f;
    inline constexpr float NearPlane = 0.1f;
    inline constexpr float FarPlane = 1000.0f;
    inline constexpr float MaxPitch = 89.0f;
    inline constexpr float MinOrbitDistance = 1.0f;
}

class Camera
{
public:
    Camera();

    void SetMode(CameraMode mode) { _mode = mode; }
    CameraMode GetMode() const { return _mode; }

    // Free-fly controls
    void MoveForward(float amount);
    void MoveRight(float amount);
    void MoveUp(float amount);
    void Rotate(float yawDelta, float pitchDelta);

    // Orbit controls
    void SetOrbitTarget(const glm::vec3& target);
    void OrbitRotate(float yawDelta, float pitchDelta);
    void OrbitZoom(float amount);

    // Matrix getters
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    // State access
    const glm::vec3& GetPosition() const { return _position; }
    const glm::vec3& GetForward() const { return _forward; }
    const glm::vec3& GetRight() const { return _right; }
    const glm::vec3& GetUp() const { return _up; }

    void SetPosition(const glm::vec3& pos) { _position = pos; }
    void SetFov(float fov) { _fov = fov; }
    void SetNearPlane(float nearDist) { _nearPlane = nearDist; }
    void SetFarPlane(float farDist) { _farPlane = farDist; }
    float GetNearPlane() const { return _nearPlane; }
    float GetFarPlane() const { return _farPlane; }

    float GetYaw() const { return _yaw; }
    float GetPitch() const { return _pitch; }

private:
    void UpdateVectors();

    CameraMode _mode;

    glm::vec3 _position;
    glm::vec3 _forward;
    glm::vec3 _right;
    glm::vec3 _up;

    float _yaw;
    float _pitch;

    // Orbit mode state
    glm::vec3 _orbitTarget;
    float _orbitDistance;

    // Projection parameters
    float _fov;
    float _nearPlane;
    float _farPlane;
};

} // namespace planets::core
