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
} // namespace CameraDefaults

// Interactive-view readout the GUI shows and edits. The camera controller writes altitude and
// speed each frame; autoRotate is the one switch both the O key and the Camera tab set.
struct CameraHud
{
    float altitude = 0.0f; // above sea level
    float speed = 0.0f;    // current fly speed, world units per second
    bool autoRotate = false;
};

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

    // Point the camera at a world-space target (FreeFly orientation). Back-solves yaw/pitch
    // so a subsequent delta Rotate stays continuous. Used for headless light-axis framing.
    void LookAt(const glm::vec3& target);

    // Seat any orientation. upHint only picks the roll; yaw/pitch are back-solved so a later
    // Rotate continues from here.
    void SetOrientation(const glm::vec3& forward, const glm::vec3& upHint);

    // Fly look about a local vertical (not always world Y): yaw turns about `up`, pitch tilts
    // the view and stops short of straight up/down.
    void Look(float yawDelta, float pitchDelta, const glm::vec3& up);

    // Keep the view direction, level the roll against a new vertical.
    void Relevel(const glm::vec3& up);

    // Orbit controls
    void SetOrbitTarget(const glm::vec3& target);
    void OrbitRotate(float yawDelta, float pitchDelta);
    // Like OrbitRotate, but with no pitch limit: yaw turns about world Y and pitch about the
    // camera's right, so a drag carries on over the poles without a flip or a roll.
    void OrbitTumble(float yawDelta, float pitchDelta);
    void OrbitZoom(float amount);
    void SetOrbitDistance(float distance);

    // Seat the orbit camera at an absolute yaw/pitch (OrbitRotate is delta-only).
    void SetOrbitAngles(float yaw, float pitch);

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
    float GetFov() const { return _fov; }
    const glm::vec3& GetOrbitTarget() const { return _orbitTarget; }
    float GetOrbitDistance() const { return _orbitDistance; }
    CameraHud& Hud() { return _hud; }
    const CameraHud& Hud() const { return _hud; }
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

    CameraHud _hud;

    // Projection parameters
    float _fov;
    float _nearPlane;
    float _farPlane;
};

} // namespace planets::core
