#pragma once

#include "Camera.h"
#include <glm/glm.hpp>

namespace planets::core
{

// The planet's size as the camera needs it. All values are in world units.
struct CameraBody
{
    float radius = 10.0f;
    float seaRadius = 10.0f;     // altitude is measured from here
    float floorRadius = 10.0f;   // the camera never goes below this (plus a margin)
    float startDistance = 20.0f; // where Reset puts the camera
};

// One frame of user input, already filtered by the GUI. All zero = no input.
struct CameraControl
{
    glm::vec3 move{0.0f};   // x right, y up, z forward, each -1..1
    float boost = 1.0f;     // fly speed factor (Shift / Ctrl)
    float zoomNotches = 0.0f;
    glm::vec2 orbitDrag{0.0f}; // pixels, y up
    glm::vec2 look{0.0f};      // pixels, y up
    float autoRotateDegPerSec = 0.0f;
};

struct CameraPose
{
    glm::vec3 position{0.0f, 0.0f, 20.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    CameraMode mode = CameraMode::Orbit;
};

// Interactive camera: Orbit (around the planet centre) and Fly, with eased velocity and zoom,
// a level horizon near the surface, a minimum clearance, and eased moves between poses. It
// reads the camera as the source of truth each frame and only writes when the user acts or a
// move is running, so a script that sets the camera directly is never fought.
class CameraController
{
public:
    void Update(float dt, const CameraBody& body, const CameraControl& input, Camera& camera);

    void ToggleMode(const CameraBody& body, Camera& camera);
    void FramePlanet(const CameraBody& body, Camera& camera);
    void ResetView(const CameraBody& body, Camera& camera, bool eased);
    // Fly (eased) to just above a point on the planet, facing the horizon.
    void FlyToPoint(const glm::vec3& surfacePoint, const CameraBody& body, Camera& camera);
    void GoTo(const CameraPose& pose, const CameraBody& body, Camera& camera);
    CameraPose CapturePose(const Camera& camera) const;
    // The mode the camera will be in once the running move ends.
    CameraMode EffectiveMode(const Camera& camera) const { return _move.active ? _move.to.mode : camera.GetMode(); }

    // Drop any eased move or velocity (a script or the turntable takes over).
    void Cancel(Camera& camera);
    bool IsMoving() const { return _move.active; }

    float& SpeedMultiplier() { return _speedMultiplier; }
    float SpeedMultiplier() const { return _speedMultiplier; }

    // Nearest hit of a ray against a sphere at the origin; false on a miss.
    static bool RaySphere(const glm::vec3& origin, const glm::vec3& dir, float radius, glm::vec3& hit);
    // World-space ray direction through a point on screen (ndc -1..1, y up).
    static glm::vec3 ScreenRay(const Camera& camera, float ndcX, float ndcY, float aspect);

    static constexpr float MinSpeedMultiplier = 0.05f;
    static constexpr float MaxSpeedMultiplier = 50.0f;

private:
    struct Move
    {
        bool active = false;
        float t = 0.0f;
        float duration = 1.0f;
        CameraPose from;
        CameraPose to;
    };

    void UpdateOrbit(float dt, const CameraBody& body, const CameraControl& input, Camera& camera);
    void UpdateFly(float dt, const CameraBody& body, const CameraControl& input, Camera& camera);
    float FlySpeedAt(const CameraBody& body, const glm::vec3& position) const;
    void StepMove(float dt, Camera& camera);
    void BeginMove(const CameraPose& to, float duration, Camera& camera);
    void EndMove(Camera& camera, bool snapToTarget);
    void SyncOrbit(Camera& camera, const glm::vec3& target) const;
    void UpdateHud(const CameraBody& body, const CameraControl& input, Camera& camera) const;

    Move _move;
    glm::vec3 _velocity{0.0f};
    glm::vec3 _lastPosition{0.0f};
    bool _haveLast = false;
    float _zoomTarget = 0.0f; // orbit distance the wheel is easing toward
    bool _zoomValid = false;
    float _speedMultiplier = 1.0f;
    float _bodyRadius = 10.0f; // last seen, for the leveling math inside EndMove
};

} // namespace planets::core
