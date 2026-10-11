#include "math/CameraController.h"
#include <glm/gtc/quaternion.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace planets::core;
using Catch::Matchers::WithinAbs;

namespace
{
CameraBody TestBody()
{
    CameraBody body;
    body.radius = 10.0f;
    body.seaRadius = 9.95f;
    body.floorRadius = 10.4f;
    body.startDistance = 20.0f;
    return body;
}

bool IsFinite(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void Run(CameraController& ctl, Camera& cam, const CameraControl& input, float seconds)
{
    for (float t = 0.0f; t < seconds; t += 1.0f / 60.0f)
        ctl.Update(1.0f / 60.0f, TestBody(), input, cam);
}
} // namespace

TEST_CASE("ResetView seats an Orbit camera at the start distance", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, /*eased=*/false);
    REQUIRE(cam.GetMode() == CameraMode::Orbit);
    REQUIRE_THAT(glm::length(cam.GetPosition()), WithinAbs(20.0, 1e-3));
}

TEST_CASE("Fly never goes below the clearance floor", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    ctl.ToggleMode(TestBody(), cam);
    REQUIRE(cam.GetMode() == CameraMode::FreeFly);

    CameraControl dive;
    dive.move = glm::vec3(0.0f, 0.0f, 1.0f); // forward, toward the planet
    Run(ctl, cam, dive, 30.0f);
    REQUIRE(glm::length(cam.GetPosition()) >= TestBody().floorRadius - 1e-4f);
}

TEST_CASE("Orbit zoom eases toward the surface and stops above the floor", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);

    CameraControl zoom;
    zoom.zoomNotches = 5.0f;
    for (int i = 0; i < 20; ++i)
        Run(ctl, cam, zoom, 0.1f);
    Run(ctl, cam, CameraControl{}, 3.0f);

    const float distance = glm::length(cam.GetPosition());
    REQUIRE(distance < 12.0f);
    REQUIRE(distance >= TestBody().floorRadius);
}

TEST_CASE("Low Fly keeps the horizon level to the surface", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    ctl.ToggleMode(TestBody(), cam);
    cam.SetPosition(glm::vec3(8.0f, 6.0f, 0.0f) * 1.3f); // about 1.3 radii out, off the Y axis
    ctl.Cancel(cam);

    CameraControl strafe;
    strafe.move = glm::vec3(1.0f, 0.0f, 0.0f);
    Run(ctl, cam, strafe, 1.0f);

    const glm::vec3 normal = glm::normalize(cam.GetPosition());
    REQUIRE(glm::dot(cam.GetUp(), normal) > 0.95f);
}

TEST_CASE("An idle controller leaves a camera set from outside alone", "[camera]")
{
    Camera cam;
    CameraController ctl;
    cam.SetMode(CameraMode::FreeFly);
    cam.SetPosition(glm::vec3(3.0f, 4.0f, 5.0f)); // inside the planet: a script may do this
    cam.LookAt(glm::vec3(0.0f));
    const glm::vec3 position = cam.GetPosition();
    const glm::vec3 forward = cam.GetForward();

    Run(ctl, cam, CameraControl{}, 1.0f);
    REQUIRE(cam.GetPosition() == position);
    REQUIRE(cam.GetForward() == forward);
}

TEST_CASE("FramePlanet ends in Orbit with the whole planet in view", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    ctl.ToggleMode(TestBody(), cam);
    cam.SetPosition(glm::vec3(0.0f, 0.0f, 10.6f));
    ctl.Cancel(cam);

    ctl.FramePlanet(TestBody(), cam);
    REQUIRE(ctl.IsMoving());
    Run(ctl, cam, CameraControl{}, 3.0f);
    REQUIRE_FALSE(ctl.IsMoving());
    REQUIRE(cam.GetMode() == CameraMode::Orbit);
    // The planet's angular radius fits inside half the vertical field of view, with a margin.
    const float halfAngle = std::asin(TestBody().radius / glm::length(cam.GetPosition()));
    REQUIRE(halfAngle < glm::radians(cam.GetFov()) * 0.5f * 0.85f);
    REQUIRE(halfAngle > glm::radians(cam.GetFov()) * 0.5f * 0.6f);
    REQUIRE(glm::dot(cam.GetForward(), -glm::normalize(cam.GetPosition())) > 0.999f);
}

TEST_CASE("FlyToPoint lands above that point, facing the horizon", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);

    const glm::vec3 point = glm::normalize(glm::vec3(1.0f, 0.5f, -0.3f)) * 9.95f;
    ctl.FlyToPoint(point, TestBody(), cam);
    Run(ctl, cam, CameraControl{}, 4.0f);

    REQUIRE(cam.GetMode() == CameraMode::FreeFly);
    const glm::vec3 normal = glm::normalize(point);
    REQUIRE(glm::dot(glm::normalize(cam.GetPosition()), normal) > 0.9999f);
    REQUIRE(glm::length(cam.GetPosition()) > TestBody().floorRadius);
    REQUIRE_THAT(glm::dot(cam.GetForward(), normal), WithinAbs(0.0, 0.01));
}

TEST_CASE("Any input cancels an eased move", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    ctl.FramePlanet(TestBody(), cam);
    REQUIRE(ctl.IsMoving());

    CameraControl drag;
    drag.orbitDrag = glm::vec2(5.0f, 0.0f);
    ctl.Update(1.0f / 60.0f, TestBody(), drag, cam);
    REQUIRE_FALSE(ctl.IsMoving());
    REQUIRE(cam.GetMode() == CameraMode::Orbit);
}

TEST_CASE("RaySphere hits the near side and misses beside the sphere", "[camera]")
{
    glm::vec3 hit;
    REQUIRE(CameraController::RaySphere(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), 10.0f, hit));
    REQUIRE_THAT(hit.z, WithinAbs(10.0, 1e-4));
    REQUIRE_FALSE(CameraController::RaySphere(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(1.0f, 0.0f, 0.0f), 10.0f, hit));
}

TEST_CASE("OrbitTumble moves like OrbitRotate away from the poles", "[camera]")
{
    Camera a;
    a.SetMode(CameraMode::Orbit);
    a.SetOrbitTarget(glm::vec3(0.0f));
    a.SetOrbitDistance(20.0f);
    a.SetOrbitAngles(30.0f, 10.0f);
    Camera b = a;
    a.OrbitRotate(2.0f, 1.5f);
    b.OrbitTumble(2.0f, 1.5f);
    REQUIRE(glm::distance(a.GetPosition(), b.GetPosition()) < 0.05f);
    REQUIRE(glm::dot(a.GetForward(), b.GetForward()) > 0.9999f);
}

TEST_CASE("A sideways orbit drag after a vertical one keeps the pole upright", "[camera]")
{
    Camera cam;
    cam.SetMode(CameraMode::Orbit);
    cam.SetPosition(glm::vec3(0.0f, 0.0f, 20.0f));
    cam.SetOrbitTarget(glm::vec3(0.0f));
    cam.SetOrbitAngles(90.0f, 0.0f);
    const glm::vec3 worldY(0.0f, 1.0f, 0.0f);

    cam.OrbitTumble(0.0f, 45.0f);
    for (int i = 0; i < 90; ++i)
    {
        cam.OrbitTumble(1.0f, 0.0f);
        REQUIRE_THAT(glm::dot(cam.GetRight(), worldY), WithinAbs(0.0, 1e-4));
    }
    REQUIRE_THAT(glm::length(cam.GetPosition()), WithinAbs(20.0, 1e-3));

    // Upside down past the pole, the same drag must still run the same way on screen.
    cam.OrbitTumble(0.0f, 100.0f);
    REQUIRE(cam.GetUp().y < 0.0f);
    const glm::vec3 before = cam.GetPosition();
    cam.OrbitTumble(10.0f, 0.0f);
    REQUIRE(glm::dot(cam.GetPosition() - before, cam.GetRight()) < 0.0f);
    REQUIRE_THAT(glm::dot(cam.GetRight(), worldY), WithinAbs(0.0, 1e-4));
}

TEST_CASE("An eased move to a rolled Orbit pose ends without a snap", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);

    CameraPose pose;
    pose.mode = CameraMode::Orbit;
    pose.position = glm::vec3(20.0f, 0.0f, 0.0f);
    pose.forward = glm::vec3(-1.0f, 0.0f, 0.0f);
    pose.up = glm::angleAxis(glm::radians(30.0f), pose.forward) * glm::vec3(0.0f, 1.0f, 0.0f);
    ctl.GoTo(pose, TestBody(), cam);

    glm::vec3 prevUp = cam.GetUp();
    glm::vec3 prevForward = cam.GetForward();
    float lastStep = 1.0f;
    for (int i = 0; i < 90 && ctl.IsMoving(); ++i)
    {
        ctl.Update(1.0f / 60.0f, TestBody(), CameraControl{}, cam);
        lastStep = (std::min)(glm::dot(prevUp, cam.GetUp()), glm::dot(prevForward, cam.GetForward()));
        prevUp = cam.GetUp();
        prevForward = cam.GetForward();
    }
    REQUIRE_FALSE(ctl.IsMoving());
    REQUIRE(cam.GetMode() == CameraMode::Orbit);
    REQUIRE(glm::dot(cam.GetForward(), pose.forward) > 0.9999f);
    REQUIRE(glm::dot(cam.GetUp(), pose.up) > 0.9999f);
    REQUIRE(lastStep > std::cos(glm::radians(1.0f)));
}

TEST_CASE("GoTo keeps a recalled pose above the floor", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);

    CameraPose pose;
    pose.mode = CameraMode::FreeFly;
    pose.position = glm::vec3(0.0f, 0.0f, 5.0f); // inside the planet
    ctl.GoTo(pose, TestBody(), cam);
    Run(ctl, cam, CameraControl{}, 3.0f);
    REQUIRE(glm::length(cam.GetPosition()) >= TestBody().floorRadius - 1e-4f);
}

TEST_CASE("GoTo with a degenerate direction falls back to facing the planet", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);

    CameraPose pose;
    pose.mode = CameraMode::FreeFly;
    pose.position = glm::vec3(30.0f, 0.0f, 0.0f);
    pose.forward = glm::vec3(0.0f);
    ctl.GoTo(pose, TestBody(), cam);
    Run(ctl, cam, CameraControl{}, 3.0f);

    REQUIRE(IsFinite(cam.GetPosition()));
    REQUIRE(IsFinite(cam.GetForward()));
    REQUIRE(IsFinite(cam.GetUp()));
    REQUIRE(glm::dot(cam.GetForward(), -glm::normalize(cam.GetPosition())) > 0.999f);
}

TEST_CASE("A long vertical orbit drag carries on over the pole without a flip", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    const float distance = glm::length(cam.GetPosition());

    CameraControl drag;
    drag.orbitDrag = glm::vec2(0.0f, -10.0f); // drag down: the camera climbs
    float topY = cam.GetPosition().y;
    glm::vec3 up = cam.GetUp();
    for (int i = 0; i < 120; ++i)
    {
        ctl.Update(1.0f / 60.0f, TestBody(), drag, cam);
        REQUIRE(glm::dot(cam.GetUp(), up) > 0.9f); // no snap between frames
        up = cam.GetUp();
        topY = (std::max)(topY, cam.GetPosition().y);
    }
    // 120 steps of 3 degrees = 360: up over the pole, down the far side and back around.
    REQUIRE(topY > distance * 0.999f);
    REQUIRE_THAT(glm::length(cam.GetPosition()), WithinAbs(distance, 1e-2));
}

TEST_CASE("Fly moves the camera with WASD/QE and turns with look", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    Run(ctl, cam, CameraControl{}, 0.5f);
    ctl.ToggleMode(TestBody(), cam);
    REQUIRE(cam.GetMode() == CameraMode::FreeFly);
    Run(ctl, cam, CameraControl{}, 0.2f);
    REQUIRE(cam.GetMode() == CameraMode::FreeFly);

    const glm::vec3 start = cam.GetPosition();
    CameraControl strafe;
    strafe.move = glm::vec3(1.0f, 0.0f, 0.0f); // D
    Run(ctl, cam, strafe, 1.0f);
    REQUIRE(glm::distance(cam.GetPosition(), start) > 1.0f);

    // Pitch up: looking straight down at the planet, yaw only spins the view, pitch must turn it.
    const glm::vec3 forward = cam.GetForward();
    CameraControl look;
    look.look = glm::vec2(0.0f, 20.0f);
    Run(ctl, cam, look, 0.2f);
    REQUIRE(glm::dot(cam.GetForward(), forward) < 0.95f);
}

TEST_CASE("WASD or a right-drag in Orbit switches to Fly and moves", "[camera]")
{
    Camera cam;
    CameraController ctl;
    ctl.ResetView(TestBody(), cam, false);
    Run(ctl, cam, CameraControl{}, 0.2f);
    REQUIRE(cam.GetMode() == CameraMode::Orbit);

    const glm::vec3 start = cam.GetPosition();
    CameraControl forward;
    forward.move = glm::vec3(0.0f, 0.0f, 1.0f); // W
    Run(ctl, cam, forward, 0.5f);
    REQUIRE(cam.GetMode() == CameraMode::FreeFly);
    REQUIRE(glm::length(cam.GetPosition()) < glm::length(start) - 0.5f);

    Camera cam2;
    CameraController ctl2;
    ctl2.ResetView(TestBody(), cam2, false);
    CameraControl look;
    look.look = glm::vec2(0.0f, -5.0f); // right-drag
    Run(ctl2, cam2, look, 0.1f);
    REQUIRE(cam2.GetMode() == CameraMode::FreeFly);
}
